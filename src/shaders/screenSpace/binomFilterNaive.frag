#version 430

//!< in-variable
in vec3 passPosition;

//!< uniforms
uniform sampler2D tex;
uniform int radius;

uniform int level;

uniform float binomWeights[33]; //max radius: 16

const int MAX_RADIUS = 16;

//!< out-variables
layout(location = 0) out vec4 fragColor;

float getWeight(int i, int j)
{
	float verticalWeight = binomWeights[i + min(radius, MAX_RADIUS)];
	float horizontalWeight = binomWeights[j + min(radius, MAX_RADIUS)];

	return verticalWeight * horizontalWeight;
}

void main() 
{
	vec4 filteredColor = vec4(0,0,0,0);
	float totalWeight = 0.0;
	for ( int i = -radius; i <= radius; i++)
	{
		for (int j = -radius; j <= radius; j ++)
		{
			vec4 currentSample = textureLodOffset( tex, passPosition.xy, float(level-1), ivec2(i,j) );
			float currentWeight = getWeight(i,j);
			filteredColor += currentSample * currentWeight;
			totalWeight += currentWeight;
		}
	}
	filteredColor /= totalWeight;

	//!< fragcolor gets transparency by uniform
    fragColor = filteredColor;
}