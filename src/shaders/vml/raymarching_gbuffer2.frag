#version 330

#define SAMPLES 128
#define SAMPLES_RCP 0.0078125
#define PI_RCP 0.31830988618379067153776752674503
#define BIAS 0.00005
#define PHI 10000000.0
#define TAU 0.0001

in vec2 passUV;

uniform sampler2D worldPosMap;
uniform sampler2D shadowMapSampler;

uniform mat4 lightView;
uniform mat4 lightProjection;
uniform vec4 lightColor;

uniform vec4 cameraPosLightSpace;
uniform vec4 color;
uniform float albedo;

// visibility function
float v(vec3 rayPositionLightSpace) {
    vec4 rayCoordLightSpace = lightProjection * vec4(rayPositionLightSpace, 1.0);
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
float executeRaymarching(vec3 x, float dl, float l) {
    // fetch whether the current position on the ray is visible form the light's perspective or not
    float v = v(x);

    // get distance of current ray postition to the light source in light view-space
    float d = length(x);
    float dRcp = (d != 0) ? 1.0/d : 0;

    // calculate anisotropic scattering
    float p = p();

    // calculate the final light contribution for the sample on the way
    float radiantFluxAttenuation = PHI * PI_RCP * 0.25 * dRcp * dRcp;
    float Li = TAU * albedo * radiantFluxAttenuation * v * p;
    float intens = Li * exp(-TAU * d) * exp(-TAU * l) * dl;
    return intens;
}

void main() {
    // calculate complete raymarching distance
    vec4 startPosLightSpace = lightView * texture(worldPosMap, passUV);
    float s = length(cameraPosLightSpace.xyz - startPosLightSpace.xyz);
    float dl = s * SAMPLES_RCP;

    // calculate the stepsize of the ray
    vec3 viewDir = normalize(cameraPosLightSpace.xyz - startPosLightSpace.xyz);
    vec3 x = startPosLightSpace.xyz;

    // total light contribution accumulated along the ray
    float vli = 0.0f;
    for (float l = s - dl; l >= 0;  l -= dl) {
        x +=  viewDir * dl;
        vli += executeRaymarching(x, dl, l);
    }
    vli *= SAMPLES_RCP;
    vli = clamp(vli, 0, 1);

    gl_FragColor =  vec4(lightColor.xyz * vli, 1.0);
}
