#version 330

uniform sampler2D uInputTex;

uniform vec4 uScale;
uniform vec4 uBias;

in vec2 passUV;

out vec4 fResult;

void main() {
  	fResult = max(vec4(0.0), texture(uInputTex, passUV) + uBias) * uScale;
}