#version 430

in vec2 passUV;

// -*- c++ -*-
/**
 Computes the near field blur.  This is included by both the horizontal
 (first) and vertical (second) passes.

 The shader produces two outputs:

 * blurResult

   A buffer that is the scene blurred with a spatially varying kernel
   kernel that attempts to make the point-spread function at each pixel
   equal to its circle of confusion radius.

   blurResult.rgb = color
   blurResult.a   = normalized coc radius

 * nearResult

   A buffer that contains only near field, blurred with premultiplied
   alpha.  The point spread function is a fixed RADIUS in this region.

   nearResult.rgb = coverage * color
   nearResult.a   = coverage

 */

uniform bool HORIZONTAL; //"1 if horizontal, 0 if vertical"

/** Maximum blur radius for any point in the scene, in pixels.  Used to
    reconstruct the CoC radius from the normalized CoC radius. */
uniform int         maxCoCRadiusPixels;

/** Source image in RGB, CoC in A. */
uniform sampler2D   blurSourceBuffer;

uniform int         nearBlurRadiusPixels;
uniform float       invNearBlurRadiusPixels;

out vec4 nearResult;
out vec4 blurResult;

/** For the second pass, the output of the previous near-field blur pass. */
uniform sampler2D  nearSourceBuffer;

bool inNearField(float cocRadius)
{
    return ( cocRadius > 0.5 );
}

float saturate(float val)
{
    return clamp(val, 0.0, 1.0);
}

const ivec2 hDirection = ivec2(1, 0);
const ivec2 vDirection = ivec2(0, 1);

void main() {

    ivec2 direction = vDirection;
    if (HORIZONTAL)
    {
        direction = hDirection;
    }

    const int KERNEL_TAPS = 6;
    float kernel[KERNEL_TAPS + 1];
    
    // 11 x 11 separated kernel weights.  This does not dictate the 
    // blur kernel for depth of field; it is scaled to the actual
    // kernel at each pixel.
    //    Custom disk-like // vs. Gaussian
    kernel[6] = 0.00; //0.00000000000000;  // Weight applied to outside-radius values
    kernel[5] = 0.50;//0.04153263993208;
    kernel[4] = 0.60;//0.06352050813141;
    kernel[3] = 0.75;//0.08822292796029;
    kernel[2] = 0.90;//0.11143948794984;
    kernel[1] = 1.00;//0.12815541114232;
    kernel[0] = 1.00;//0.13425804976814;
    
    // Accumulate the blurry image color
    blurResult.rgb  = vec3(0.0f);
    float blurWeightSum = 0.0f;
    
    // Accumulate the near-field color and coverage
    nearResult = vec4(0.0f);
    float nearWeightSum = 0.000f;
    
    // Location of the central filter tap (i.e., "this" pixel's location)
    // Account for the scaling down to 25% of original dimensions during blur
    ivec2 A = ivec2(gl_FragCoord.xy * (direction  * 3 + ivec2(1) ) );
    
    float packedA = texelFetch(blurSourceBuffer, A, 0).a;
    // float packedA = texelFetch(cocMap, ivec2(gl_FragCoord.xy* 4) , 0).x;
    float r_A = /* ( */ packedA /* * 2.0 - 1.0) * maxCoCRadiusPixels */;
    
    // Map r_A << 0 to 0, r_A >> 0 to 1
    float nearFieldness_A = saturate(r_A * 4.0);
    
    for (int delta = -maxCoCRadiusPixels; delta <= maxCoCRadiusPixels; ++delta) {
        // Tap location near A
        ivec2 B = A + (direction * delta);

        // Packed values
        ivec2 B_texelCoords = clamp(B, ivec2(0), textureSize(blurSourceBuffer, 0) - ivec2(1));
        vec4 blurInput = texelFetch(blurSourceBuffer, B_texelCoords, 0);

        // Signed kernel radius at this tap, in pixels
        // float r_B = (blurInput.a * 2.0 - 1.0) * float(maxCoCRadiusPixels);
        float r_B = blurInput.a;

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Compute blurry buffer

        float weight = 0.0;
        
        float wNormal  = 
            // Only consider mid- or background pixels (allows inpainting of the near-field)
            float(! inNearField(r_B)) * 
            
            // Only blur B over A if B is closer to the viewer (allow 0.5 pixels of slop, and smooth the transition)
            // This term avoids "glowy" background objects but leads to aliasing under 4x downsampling
            // saturate(abs(r_A) - abs(r_B) + 1.5) *
            
            // Stretch the kernel extent to the radius at pixel B.
            kernel[clamp(int(float(abs(delta) * (KERNEL_TAPS - 1)) / (0.001 + abs(r_B * 0.8))), 0, KERNEL_TAPS)];

        weight = mix(wNormal, 1.0, nearFieldness_A);

        // far + mid-field output 
        blurWeightSum  += weight;
        blurResult.rgb += blurInput.rgb * weight;
        
        ///////////////////////////////////////////////////////////////////////////////////////////////
        // Compute near-field super blurry buffer
        
        vec4 nearInput;
        if (HORIZONTAL){
            // On the first pass, extract coverage from the near field radius
            // Note that the near field gets a box-blur instead of a kernel 
            // blur; we found that the quality improvement was not worth the 
            // performance impact of performing the kernel computation here as well.

            // Curve the contribution based on the radius.  We tuned this particular
            // curve to blow out the near field while still providing a reasonable
            // transition into the focus field.
            nearInput.a = float(abs(delta) <= r_B) * saturate(r_B * invNearBlurRadiusPixels * 4.0);
            // Optionally increase edge fade contrast in the near field
            nearInput.a *= nearInput.a; nearInput.a *= nearInput.a;

            // Compute premultiplied-alpha color
            nearInput.rgb = blurInput.rgb * nearInput.a;
        }
        else
        {
            // On the second pass, use the already-available alpha values
            nearInput = texelFetch(nearSourceBuffer, clamp(B, ivec2(0), textureSize(nearSourceBuffer, 0) - ivec2(1)), 0);
        }    

        // We subsitute the following efficient expression for the more complex: weight = kernel[clamp(int(float(abs(delta) * (KERNEL_TAPS - 1)) * invNearBlurRadiusPixels), 0, KERNEL_TAPS)];
        weight =  float(abs(delta) < nearBlurRadiusPixels);
        nearResult += nearInput * weight;
        nearWeightSum += weight;
    }
    
    if (HORIZONTAL)
    {
        // Retain the packed radius on the first pass.  On the second pass it is not needed.
        blurResult.a = packedA;
    } else{
        blurResult.a = 1.0;
    }

    // Normalize the blur
    blurResult.rgb /= blurWeightSum;
    nearResult     /= max(nearWeightSum, 0.00001); 
}