#version 430

in vec2 passSampleCoord;

out vec4 fragmentColor;

uniform vec4 lightData;
uniform sampler2D depthTexture;

void main() {

    vec4 depthSample = texture(depthTexture, passSampleCoord);
    float sampleDepth = depthSample.x;
    float test = step( lightData.z, sampleDepth ); // test = lightData.z < depth ? 1.0 : 0.0
    if (test == 0)
    {
        discard;
    }
    fragmentColor = vec4(vec3(sampleDepth),1.0) ;
}