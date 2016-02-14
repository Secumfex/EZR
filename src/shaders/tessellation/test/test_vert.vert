#version 430

layout(location = 0) in vec2 position;
//out vec3 passPosition;

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main () {
	float height = rand(position) / 5.0f;
	//passPosition = vec3(position.xy, height);
	gl_Position = vec4(position.xy, height, 1);
}