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
	vec4 texColor = texture(tex, passPosition.xy) + (texture(addTex, passPosition.xy) * strength);

	//!< fragcolor gets transparency by uniform
    fragColor = texColor;
}