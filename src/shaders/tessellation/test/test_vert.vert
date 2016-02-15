#version 430

layout(location = 0) in vec2 position;
uniform sampler2D terrain;

//out vec3 passPosition;

	//src: http://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float make_simple_interpolation(vec3 rgb) {
	return (rgb.x + rgb.y + rgb.z) / 3.0;
}
void main () {
	float height = make_simple_interpolation(texture(terrain, position).xyz);
	//float height = abs(rand(position)) / 12.0f;
	//passPosition = vec3(position.xy, height);

	gl_Position = vec4(position.xy, height, 1);
}