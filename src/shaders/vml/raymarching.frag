#version 330

#define NUM_SAMPLES 256
#define NUM_SAMPLES_RCP 0.0078125
//#define TAU 0.0001
//#define ALBEDO 1
//#define PHI 10000000.0
#define PI_RCP 0.31830988618379067153776752674503
#define BIAS 0.005

in vec4 startPosLightSpace;

uniform sampler2D shadowMapSampler;
uniform int numberOfSamples;
uniform float phi;
uniform mat4 lightProjection;
uniform vec4 lightColor;
uniform vec4 cameraPosLightSpace;
uniform vec4 cameraViewDirInvLightSpace;
uniform vec4 color;
uniform float tau;
uniform float albedo;

// visibility function
float v(vec4 rayPositionLightSpace) {
    vec4 rayCoordLightSpace = lightProjection * rayPositionLightSpace;
    rayCoordLightSpace.xyz /= rayCoordLightSpace.w;
    rayCoordLightSpace.xyz /= 2;
    rayCoordLightSpace.xyz += 0.5;

    float v = 1;
    if (texture(shadowMapSampler, rayCoordLightSpace.xy).z < rayCoordLightSpace.z - BIAS) {
        v = 0;
    }
    return v;
}

// phase function for anisotropic scattering
float p() {
    return 1.0f;
}

// calculate the lightintensity of the current sample at position rayPositionLightSpace
float executeRaymarching(vec4 rayPositionLightSpace, float stepSize, float l) {
    // fetch whether the current position on the ray is visible form the light's perspective or not
    float v = v(rayPositionLightSpace);

    // get distance of current ray postition to the light source in light view-space
    float d = length(rayPositionLightSpace.xyz);
    float dRcp = (d != 0) ? 1.0/d : 1000000;

    // calculate anisotropic scattering
    float p = p();

    // calculate the final light contribution for the sample on the way
    float radiantFluxAttenuation = phi * 0.25 * PI_RCP * dRcp * dRcp;
    float Li = tau * albedo * radiantFluxAttenuation * v * exp(-tau * d) * p;
    float intens = Li * exp(-tau * l) * stepSize;
    return intens;
}

void main() {

    // fallback if we can't find a tighter limit
    float raymarchDistanceLimit = 999999.0;

    // calculate number of samples
    float sampleNum = pow(2, numberOfSamples);
    float sampleNumInv = 1 / sampleNum;

    // calculate complete raymarching distance
    float raymarchDistance = length(cameraPosLightSpace.xyz - startPosLightSpace.xyz);
    float stepSize = raymarchDistance * (sampleNumInv);
    vec4 rayPositionLightSpace = startPosLightSpace;

    // total light contribution accumulated along the ray
    float vli = 0.0f;
    float l = raymarchDistance;
    for (float t = 0.0; t < 1.0; t= t+ sampleNumInv)
    {
        l = raymarchDistance - t * stepSize;
        vec4 currentPos = vec4(startPosLightSpace.xyz + t * (cameraPosLightSpace.xyz - startPosLightSpace.xyz),1.0);
        vli += executeRaymarching(currentPos, stepSize, l);
    }

    vli /= sampleNum;

    gl_FragColor =  vec4(lightColor.xyz * vli, 1);
}
