#version 430

//!< in-variable
in vec3 passPosition;

//!< uniforms
uniform sampler2D uvwFront;
uniform sampler2D uvwBack;

uniform sampler2D gaussPyramide1;
uniform sampler2D laplacePyramide1;

uniform sampler2D gaussPyramide2;
uniform sampler2D laplacePyramide2;

// uniform float t;
uniform int gaussBaseLevel;
uniform int[10] laplaceLevels;

//!< out-variables
layout(location = 0) out vec4 fragColor;

void main() 
{
	//fragCoord
	vec4 endUVW    = texture(uvwFront, passPosition.xy); 
	vec4 startUVW  = texture(uvwBack, passPosition.xy); 

	if ( startUVW.a == 0 || startUVW.a == 0)
	{
		discard;
	}

	// begin raycasting slices
	vec3 rayDir = endUVW.xyz - startUVW.xyz;
	vec3 sliceDirection = normalize(vec3(0,rayDir.y,0));

	float numSlices = 20.0;
	float nextSlice = floor( startUVW.y * numSlices ) + 1.0;
	float stepSize = 0.01;
//	float parameterStepSize = stepSize / length(endUVW.xyz - startUVW.xyz); // necessary parametric steps to get from start to end
	float parameterStepSize = ( (nextSlice / numSlices) - startUVW.y ) / rayDir.y + 0.01;

	vec4 reconstructed = vec4(0.0, 0.0, 0.0, 0.0);

	for (float t = 0.0; t < 1.0 + (0.5 * parameterStepSize); t += parameterStepSize)
	{
		vec3 curUVW = startUVW.xyz + t * rayDir;
		float curSlice = floor( curUVW.y * numSlices);

		if ( (sliceDirection.y > 0.0 && curSlice >= nextSlice) || (sliceDirection.y < 0.0 && curSlice <= nextSlice) )
		{
			// vec4 gaussbase1 = textureLod( gaussPyramide1, curUVW.xy, float(gaussBaseLevel));
			// vec4 gaussbase2 = textureLod( gaussPyramide2, curUVW.xy, float(gaussBaseLevel));
			
			// vec4 curInterpolation = mix(gaussbase1, gaussbase2, curUVW.z);		
			vec4 curInterpolation = vec4(0.0);		

			for (int i = 0; i < laplaceLevels.length(); i++)
			{
				if ( laplaceLevels[i] != 0)
				{
					vec4 laplace1 = textureLod( laplacePyramide1, curUVW.xz, float(i)) * 4.0;
					vec4 laplace2 = textureLod( laplacePyramide2, curUVW.xz, float(i)) * 4.0;

					curInterpolation += mix(laplace1, laplace2, curUVW.y);
				}
			}

			reconstructed = mix(reconstructed, curInterpolation, 0.2  * length(curInterpolation.xyz) );
			// reconstructed = mix(reconstructed, curInterpolation, curInterpolation.a );

			nextSlice += sliceDirection.y;

			parameterStepSize = ( (nextSlice/ numSlices) - curUVW.y ) / rayDir.y + 0.01; 
		}
	}


	// for (int i = 0; i < laplaceLevels.length(); i++)
	// {
	// 	if ( laplaceLevels[i] != 0)
	// 	{
	// 		vec4 laplace1 = textureLod( laplacePyramide1, passPosition.xy, float(i));
	// 		vec4 laplace2 = textureLod( laplacePyramide2, passPosition.xy, float(i));

	// 		reconstructed += mix(laplace1, laplace2, t);
	// 	}
	// }

	fragColor = vec4( reconstructed.rgb, 1.0);
}