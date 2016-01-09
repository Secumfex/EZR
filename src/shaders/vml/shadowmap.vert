#version 330

layout(location = 0) in vec4 positionAttribute;

uniform mat4 lightMVP;

void main() {
	gl_Position = lightMVP * positionAttribute;
}
