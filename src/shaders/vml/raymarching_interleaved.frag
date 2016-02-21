#version 330

#define PI_RCP 0.31830988618379067153776752674503
#define BIAS 0.005
#define PIXELTILESIZE 8 // an 8x8 pixel tile

in vec4 startPosLightSpace;
in vec2 passUV;

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

    vec2 dim = textureSize(shadowMapSampler, 0);
    float pixelBlockSize = PIXELTILESIZE * PIXELTILESIZE;

    // get position of the fragment within the pixelblock
    vec2 pixelPosition = vec2(gl_FragCoord);
    pixelPosition.xy = mod(pixelPosition.xy,PIXELTILESIZE);
    float index = PIXELTILESIZE * pixelPosition.y + pixelPosition.x;

    // randomize order of pixels in a tile
    vec2 tilePos = round(pixelPosition / PIXELTILESIZE);
    float tileIndex = tilePos.y * dim.x + tilePos.x;
    tileIndex *= 67;
    index = mod(index + tileIndex,pixelBlockSize);

    // calculate number of samples
    float sampleNum = pow(2, numberOfSamples);
    float totalSampleNum = sampleNum * pixelBlockSize;

    // calculate complete raymarching distance
    float raymarchDistance = length(cameraPosLightSpace.xyz - startPosLightSpace.xyz);
    float stepSize = raymarchDistance / totalSampleNum;

    // calculate the stepsize of the ray
    vec3 viewDir = normalize(cameraPosLightSpace.xyz - startPosLightSpace.xyz);
    vec3 rayDir = viewDir * stepSize * pixelBlockSize;

    // offset starting position
    vec3 offset = viewDir  * index * stepSize;
    vec4 rayPositionLightSpace = vec4(startPosLightSpace.xyz + offset, 1.0) ;

    // total light contribution accumulated along the ray
    float vli = 0.0f;
    for (float i = 0; i <sampleNum; i++) {
        float t = i / (sampleNum-1);
        //float distanceToCamera = raymarchDistance - (t*raymarchDistance);
        vec4 currentPos = vec4(rayPositionLightSpace.xyz + i * rayDir, 1.0);
        float distanceToCamera = length(cameraPosLightSpace.xyz - currentPos.xyz);
        vli += executeRaymarching(currentPos, stepSize, distanceToCamera);
    }

    vli /= sampleNum;

    gl_FragColor =  vec4(lightColor.xyz * vli, 1.0);
}
