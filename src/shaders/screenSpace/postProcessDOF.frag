#version 430

in vec2 passUV;

uniform sampler2D colorMap;
uniform sampler2D normalMap;
uniform sampler2D positionMap;

uniform float strength;

out vec4 fragmentColor;

void main() {
    vec4 position = texture(positionMap, passUV);
    if (position.a == 0.0) { discard; }
    vec4 normal =   texture(normalMap, passUV);
    vec4 color =    texture(colorMap, passUV);
    
    float dist = length(position.xyz);
    dist = dist;

    int samples = 16;
    int validSamples = 1;
    float stepRad = 2.0 * 3.14159 / float(samples);
    for (int i = 0; i < samples; i++)
    {   
        vec2 off = vec2(sin(stepRad * float(i)), cos(samples * float(i))) * dist / 800.0f * strength * (cos(100.0 * float(i)) * 0.5 + 0.5);
        if ( length(texture(positionMap, passUV + off).xyz) <= dist )
        {
            color += texture(colorMap, passUV + off);
            validSamples ++;
        }
    }
    color /= float(validSamples);

fragmentColor = vec4(color);
}