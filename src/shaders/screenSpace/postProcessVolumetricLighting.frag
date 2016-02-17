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
//	vec4 texColor = mix(source, vml, vml.x);
	vec4 mixed = mix(vml, source, vml.r);
	vec4 texColor = mix(source, mixed, strength);

	//!< fragcolor gets transparency by uniform
    fragColor = texColor;
}