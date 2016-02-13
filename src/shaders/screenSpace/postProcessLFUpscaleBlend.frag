#version 330

	uniform sampler2D uInputTex; // source image
	uniform sampler2D uLensFlareTex; // input from the blur stage
	uniform sampler2D uLensDirtTex; // full resolution dirt texture

	uniform sampler2D uLensStarTex; // diffraction starburst texture
	
	uniform mat3 uLensStarMatrix; // transforms texcoords (turn them according to camera angle)

	uniform float strength;

	in vec2 passUV;

	out vec4 fResult;

	void main() {
		vec4 lensMod = texture(uLensDirtTex, passUV);

		vec2 lensStarTexcoord = (uLensStarMatrix * vec3(passUV, 1.0)).xy;	
		// vec2 lensStarTexcoord = (uLensStarMatrix * vec3(passUV - vec2(0.5,0.5), 1.0)).xy + vec2(0.5,0.5);

		// vec2 lensStarTexcoord = passUV;
		lensMod += texture(uLensStarTex, lensStarTexcoord);

		vec4 lensFlare = texture(uLensFlareTex, passUV) * lensMod;
		fResult = texture(uInputTex, passUV) + lensFlare * strength;
	}