#version 330

//!< in-variable
in vec2 passUVCoord;

//!< uniforms
uniform sampler2D tex;

//!< out-variables
layout(location = 0) out vec4 fragColor;

void main() 
{
	vec4 texColor = texture(tex, passUVCoord);
    fragColor = vec4(texColor.rgb, texColor.a);
}