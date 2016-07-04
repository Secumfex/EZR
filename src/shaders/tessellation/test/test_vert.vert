#version 430

layout(location = 0) in vec2 position;
uniform sampler2D terrain;

float make_simple_interpolation(vec3 rgb) {
	return (rgb.x + rgb.y + rgb.z) / 3.0;
}
void main () {
	//float height = make_simple_interpolation(texture(terrain, position).xyz);
	// float height = texture(terrain, position).x;
	gl_Position = vec4(position.x, 0.0, position.y, 1);
}