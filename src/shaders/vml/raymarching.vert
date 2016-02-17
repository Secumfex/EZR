#version 330

layout(location = 0) in vec4 positionAttribute;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightView;

out vec4 startPosLightSpace;
out vec2 passUV;

void main() {
	gl_Position =  projection * view * model * positionAttribute;
	startPosLightSpace = lightView * model * positionAttribute;
	passUV = positionAttribute.xy;
}
