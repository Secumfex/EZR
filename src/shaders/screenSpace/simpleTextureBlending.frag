#version 430

//!< in-variable
in vec3 passPosition;

//!< uniforms
uniform sampler2D tex1;
uniform sampler2D tex2;

uniform float t;

//!< out-variables
layout(location = 0) out vec4 fragColor;

void main() 
{
	vec4 texColor1 = texture( tex1, passPosition.xy );
	vec4 texColor2 = texture( tex2, passPosition.xy );
	
	vec4 reconstructed = mix(texColor1, texColor2, t);

	fragColor = vec4( reconstructed.rgb * 4.0, 1.0);
	// fragColor = vec4( reconstructed.a);
}