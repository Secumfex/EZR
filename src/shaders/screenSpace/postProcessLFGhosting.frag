#version 330

uniform sampler2D uInputTex;
uniform sampler1D uLensColor;

uniform int uGhosts; // number of ghost samples
uniform float uGhostDispersal; // dispersion factor
uniform float uHaloWidth; // dispersion factor

noperspective in vec2 passUV;

out vec4 fResult;

void main() {
	vec2 texcoord = - passUV + vec2(1.0);
	vec2 texelSize = 1.0 / vec2(textureSize(uInputTex, 0));

	// ghost vector to image centre:
	vec2 ghostVec = (vec2(0.5) - texcoord) * uGhostDispersal;

	// sample ghosts:  
	vec4 result = vec4(0.0);
	for (int i = 0; i < uGhosts; ++i) 
	{ 
		vec2 offset = fract(texcoord + ghostVec * float(i));

		float weight = length(vec2(0.5) - offset) / length(vec2(0.5));
		weight = pow(1.0 - weight, 10.0);

		result += texture(uInputTex, offset);
	}
	result *= texture(uLensColor, length(vec2(0.5) - texcoord) / length(vec2(0.5)));

	// sample halo:
	vec2 haloVec = normalize(ghostVec) * uHaloWidth;
	float weight = length(vec2(0.5) - fract(texcoord + haloVec)) / length(vec2(0.5));
	weight = pow(1.0 - weight, 5.0);
	result += texture(uInputTex, texcoord + haloVec) * weight;

	fResult = result;
}