#version 330

#define NUM_SAMPLES 128
#define NUM_SAMPLES_RCP 0.0078125
#define TAU 0.0001
#define PHI 10000000.0
#define PI_RCP 0.31830988618379067153776752674503

void executeRaymarching() {
    rayPositionLight.xyz += stepSize * invViewDirLight.xyz;

    // hier is something missing

    // fetch whether the current position on the ray is visible form the light's perspective or not
    float3 shadowTerm = getShadowTerm(shadowMapSampler, shadowMapSamplerState, rayPositionLightSS.xyz).xxx;

    // get distance of current ray postition to the light source in light view-space
    float d = length(rayPositionLight.xyz);
    float dRcp = rcp(d);

    // calculate the final light contribution for the sample on the way
    float3 intens = TAU * (shadowTerm * (PHI * 0.25 * PI_RCP) * dRcp * dRcp) * exp(-d * TAU) * exp(-1 * TAU) * stepSize;

    // add the light contribution to the total contribution of the ray
    vli += intens;
}

FRAGMENT_OUT  main(VERTEX_OUTPUT f_in) {

    // fallback if we can't find a tighter limit
    float raymarchDistanceLimit = 999999.0;

    // here is something missing

    // reduce noise by truncating the start position between 0 and the reymarchDistanceLLimit
    float raymarchDistance = trunc(clamp(length(cameraPositionLight.xyz - positionLight.xyz), 0.0, raymarchDistanceLimit));

    // total light contribution accumulated along the ray
    float vli = 0.0f;

    for (float l = raymarchDistance; l > stepSize; l -= stepSize) {
        executeRaymarching();
    }

    f_out.color.rgb = light_color_diffuse.rgb * vli;
	return f_out;
}
