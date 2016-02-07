#version 430

layout(location = 0) in vec4 positions;
out vec3 passPosition;

void main () {
	passPosition = positions.xyz;
	gl_Position = positions;
}