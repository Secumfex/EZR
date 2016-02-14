#version 430

layout(location = 0) in vec2 position;
//out vec3 passPosition;

void main () {
	float height = 0.0;
	//passPosition = vec3(position.xy, height);
	gl_Position = vec4(position.xy, height, 1);
}