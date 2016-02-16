#version 430

layout(location = 0) in vec4 positions;
uniform sampler2D terrain;

void main(void){
    vec2 texcoord = positions.xy;
//    float height = texture(terrain, texcoord).a;
    float height = texture(terrain, texcoord).z;

    vec4 displaced = vec4(
        positions.x, positions.y,
        height, 1.0);
		
    gl_Position = displaced;
}