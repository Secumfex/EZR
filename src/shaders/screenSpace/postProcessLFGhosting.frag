#version 330

uniform sampler2D uInputTex;
uniform sampler1D uLensColor;

uniform int uGhosts; // number of ghost samples
uniform float uGhostDispersal; // dispersion factor
uniform float uHaloWidth; // dispersion factor
uniform float uDistortion; // chromatic abberation

in vec2 passUV;

out vec4 fResult;

// chromatic distortion
vec3 textureDistorted(
  sampler2D tex,
  vec2 texcoord,
  vec2 direction, // direction of distortion
  vec3 distortion // per-channel distortion factor
) 
{
  return vec3(
     texture(tex, texcoord + direction * distortion.r).r,
     texture(tex, texcoord + direction * distortion.g).g,
     texture(tex, texcoord + direction * distortion.b).b
  );
}

void main() {
	vec2 texcoord = - passUV + vec2(1.0);
	vec2 texelSize = 1.0 / vec2(textureSize(uInputTex, 0));

	// ghost vector to image centre:
	vec2 ghostVec = (vec2(0.5) - texcoord) * uGhostDispersal;

	vec3 distortion = vec3(-texelSize.x * uDistortion, 0.0, texelSize.x * uDistortion);

	vec2 direction = normalize(ghostVec);

	// sample ghosts:  
	vec4 result = vec4(0.0);
	for (int i = 0; i < uGhosts; ++i) 
	{ 
		vec2 offset = fract(texcoord + ghostVec * float(i));

		float weight = length(vec2(0.5) - offset) / length(vec2(0.5));
		weight = pow(1.0 - weight, 10.0);

		// result += texture(uInputTex, offset);
		result.rgb += textureDistorted(uInputTex, offset, direction.xy, distortion);
	}
	result *= texture(uLensColor, length(vec2(0.5) - texcoord) / length(vec2(0.5)));

	// sample halo:
	vec2 haloVec = direction * uHaloWidth;
	float weight = length(vec2(0.5) - fract(texcoord + haloVec)) / length(vec2(0.5));
	weight = pow(1.0 - weight, 5.0);
	// result += texture(uInputTex, texcoord + haloVec) * weight;
	result.xyz += textureDistorted(uInputTex, texcoord + haloVec, direction.xy, distortion) * weight;

	fResult = result;
}