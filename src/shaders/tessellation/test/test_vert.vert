#version 430

in vec4 position;
out vec3 passPosition;

void main () {
	passPosition = position.xyz;
	gl_Position = position;
}