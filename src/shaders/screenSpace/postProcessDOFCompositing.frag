#version 430

in vec2 passUV;

uniform sampler2D  sharpFocusField; // full res COC output
uniform sampler2D  blurryNearField; // quarter res Coverage in alpha
uniform sampler2D  blurryFarField;  // quarter res

uniform float maxCoCRadiusPixels;
uniform float farRadiusRescale;


out vec4 fragmentColor;

const float coverageBoost = 1.5;


bool inNearField(float cocRadius)
{
    return ( cocRadius > 0.5 );
}

float saturate(float val)
{
    return clamp(val, 0.0, 1.0);
}

void main() {

    vec4 sharpColor = texture(sharpFocusField, passUV);
    vec4 blurryNearColor = texture(blurryNearField, passUV);
    vec4 blurryFarColor = texture(blurryFarField, passUV);

    vec3 sharp = sharpColor.rgb;
    vec3 blurred = blurryFarColor.rgb;

    float normRadius = sharpColor.a / maxCoCRadiusPixels;
    normRadius *= (normRadius < 0.0) ? farRadiusRescale : 1.0;

    // Boost the blur factor
    //normRadius = clamp(normRadius * 2.0, -1.0, 1.0);

    if (coverageBoost != 1.0) {
        float a = saturate(coverageBoost * blurryNearColor.a);
        blurryNearColor.rgb = blurryNearColor.rgb * (a / max(blurryNearColor.a, 0.001f));
        blurryNearColor.a = a;
    }

    if (normRadius > 0.1) {
        normRadius = min(normRadius * 1.5, 1.0);
    }

    sharp = mix(sharp, blurred, abs(normRadius)) * (1.0 - blurryNearColor.a) + blurryNearColor.rgb;    

    fragmentColor = vec4(sharp,1.0) ;
}