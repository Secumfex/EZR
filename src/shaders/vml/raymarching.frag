#version 330

#define NUM_SAMPLES 128
#define NUM_SAMPLES_RCP 0.0078125
#define TAU 0.0001
#define ALBEDO 1
#define PHI 10000000.0
#define PI_RCP 0.31830988618379067153776752674503
#define BIAS 0.00000000000001

in vec4 startPosLightSpace;

uniform sampler2D shadowMapSampler;
uniform mat4 lightProjection;
uniform vec4 lightColor;
uniform vec4 cameraPosLightSpace;
uniform vec4 cameraViewDirInvLightSpace;
uniform vec4 color;

// visibility function
float v(vec4 rayPositionLightSpace) {
    vec4 rayCoordLightSpace = lightProjection * rayPositionLightSpace;
    rayCoordLightSpace.xyz /= rayCoordLightSpace.w;
    rayCoordLightSpace.xyz /= 2;
    rayCoordLightSpace.xyz += 0.5;

    //rayCoordLightSpace.xy = clamp(rayCoordLightSpace.xy, 0.0, 1.0);

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
    float dRcp = (d != 0) ? 1/d : 1000000;

    // calculate anisotropic scattering
    float p = p();

    // calculate the final light contribution for the sample on the way
    float radiantFluxAttenuation = PHI * 0.25 * PI_RCP * dRcp * dRcp;
    float Li = TAU * ALBEDO * radiantFluxAttenuation * v * exp(-TAU * d) * p;
    float intens = Li * exp(-TAU * l) * stepSize;
    return intens;
}

void main() {

    // fallback if we can't find a tighter limit
    float raymarchDistanceLimit = 999999.0;

    // here is something missing

    // reduce noise by truncating the start position between 0 and the reymarchDistanceLLimit
    //float raymarchDistance = trunc(clamp(length(cameraPosLightSpace.xyz - startPosLightSpace.xyz), 0.0, raymarchDistanceLimit));
    float raymarchDistance = length(cameraPosLightSpace.xyz - startPosLightSpace.xyz);
    float stepSize = raymarchDistance * NUM_SAMPLES_RCP;
    vec4 rayPositionLightSpace = startPosLightSpace;

    // total light contribution accumulated along the ray
    float vli = 0.0f;

    for (float l = raymarchDistance; l > stepSize; l -= stepSize) {
        // add the light contribution to the total contribution of the ray
        rayPositionLightSpace.xyz += stepSize * cameraViewDirInvLightSpace.xyz;
        vli += executeRaymarching(rayPositionLightSpace, stepSize, l);
    }

    vli /= NUM_SAMPLES;

    gl_FragColor =  vec4(lightColor.xyz * vli, 1);

}
