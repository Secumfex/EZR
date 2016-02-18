#version 330

/*
* Simple fragmentshader that adds a texture ontop of the other
*/

//!< in-variable
in vec3 passPosition;

//!< uniforms
uniform sampler2D tex;
uniform sampler2D addTex;

uniform float strength;

//!< out-variables
layout(location = 0) out vec4 fragColor;

void main() 
{
	vec4 vml = texture(addTex, passPosition.xy);
	vec4 source = texture(tex, passPosition.xy);
	float intensity = vml.x;
	float weight = sqrt(intensity) - 0.1;
	vec4 white = vec4(1, 1, 1, 1);
	vec4 texColor = mix(source, white, weight);

	//!< fragcolor gets transparency by uniform
    fragColor = texColor;
}