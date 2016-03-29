#version 430

//!< in-variable
in vec3 passPosition;

//!< uniforms
uniform sampler2D gaussPyramide1;
uniform sampler2D laplacePyramide1;

uniform sampler2D gaussPyramide2;
uniform sampler2D laplacePyramide2;

uniform float t;
uniform int gaussBaseLevel;
uniform int[10] laplaceLevels;

//!< out-variables
layout(location = 0) out vec4 fragColor;

void main() 
{
	vec4 gaussbase1 = textureLod( gaussPyramide1, passPosition.xy, float(gaussBaseLevel));
	vec4 gaussbase2 = textureLod( gaussPyramide2, passPosition.xy, float(gaussBaseLevel));
	
	vec4 reconstructed = mix(gaussbase1, gaussbase2, t);

	for (int i = 0; i < laplaceLevels.length(); i++)
	{
		if ( laplaceLevels[i] != 0)
		{
			vec4 laplace1 = textureLod( laplacePyramide1, passPosition.xy, float(i));
			vec4 laplace2 = textureLod( laplacePyramide2, passPosition.xy, float(i));

			reconstructed += mix(laplace1, laplace2, t);
		}
	}

	fragColor = vec4( reconstructed.rgb, 1.0);
}