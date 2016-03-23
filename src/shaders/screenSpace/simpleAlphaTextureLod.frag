#version 330

/*
* Simple fragmentshader that can modifie the alpha value of the fragcolor. 
*/

//!< in-variable
in vec3 passPosition;

//!< uniforms
uniform sampler2D tex;
uniform float transparency;
uniform float lod	;

//!< out-variables
layout(location = 0) out vec4 fragColor;

void main() 
{
	vec4 texColor = textureLod(tex, passPosition.xy, lod);

	//!< fragcolor gets transparency by uniform
    fragColor = vec4(texColor.rgb, texColor.a * (1.0 - transparency));
}