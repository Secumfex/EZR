#version 430

in vec2 passUV;

uniform sampler2D colorMap;
uniform sampler2D cocMap;

out vec4 focusAndFarField;
out vec4 nearFieldWithAlpha;

bool inNearField(float cocRadius)
{
    if ( cocRadius > 0.5 )
    {
        return true;
    }
    else
    {
        return false;
    }
}

float saturate(float val)
{
    return min(1.0,max(0.0,val));
}

void main() {
    vec4 color =    texture(colorMap, passUV);
    float radius = abs(texture(cocMap, passUV).x);

    int samples = 16;
    int validSamples = 1;
    float stepRad = 2.0 * 3.14159 / float(samples);
    // if ( radius > 0.5 || radius < -0.5)
    // {
    for (int i = 0; i < samples; i++)
    {   
        vec2 off = vec2( sin(stepRad * float(i)), cos(samples * float(i))) * radius / 800.0f;
        float sampleRadius = texture(cocMap, passUV + off).x;
        // if ( sampleRadius >= radius)
        // {
        // float wNormal =  float ( !inNearField(sampleRadius) ) *
        // saturate( abs(radius) - abs(sampleRadius) + 1.5) *        
        color += texture(colorMap, passUV + off);
        validSamples ++;
        // }
    }
    color /= float(validSamples);
    // }

    focusAndFarField = color;
    nearFieldWithAlpha = color;
}