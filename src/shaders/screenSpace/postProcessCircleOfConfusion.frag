#version 430

in vec2 passUV;

uniform sampler2D colorMap;
uniform sampler2D depthMap;

uniform float strength;

uniform vec2 invRenderTargetSize;


out vec4 fragmentColor;

void main() {
    vec4 position = texture(depthMap, passUV);
    if (position.a == 0.0) { discard; }
	vec4 color = texture(colorMap, passUV);
	fragmentColor = position;
}    