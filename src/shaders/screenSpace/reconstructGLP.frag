#version 430

//!< in-variable
in vec3 passPosition;

//!< uniforms
uniform sampler2D gaussPyramide;
uniform sampler2D laplacePyramide;

uniform int gaussBaseLevel;
uniform int[10] laplaceLevels;

//!< out-variables
layout(location = 0) out vec4 fragColor;

void main() 
{
	vec4 reconstructed = textureLod( gaussPyramide, passPosition.xy, float(gaussBaseLevel));
	float samples = 1.0;
	
	for (int i = 0; i < laplaceLevels.length(); i++)
	{
		if ( laplaceLevels[i] != 0)
		{
			reconstructed += textureLod( laplacePyramide, passPosition.xy, float(i));
			samples += 1.0;
		}
	}
	reconstructed /= samples;

	fragColor = vec4( reconstructed.rgb, 1.0);

	// fragColor = texture(gaussPyramide, passPosition.xy);
}