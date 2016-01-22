#version 430

in vec2 passUV;

uniform sampler2D tex;
uniform float depth;
uniform float power;
uniform float intensity;

out vec4 fragmentColor;

void main() {
	vec4 bloom = vec4(0.0);
	for(float l = 4.0; l <= 4.0 + depth; l=l+1.0)
	{
		    vec4 glow_color = textureLod(tex, passUV, l);
		    bloom += pow(glow_color, vec4(power));
	}
	bloom *= intensity;
    
     fragmentColor = bloom;
}