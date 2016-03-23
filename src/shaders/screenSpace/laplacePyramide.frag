#version 430

//!< in-variable
in vec3 passPosition;

//!< uniforms
uniform sampler2D tex;

uniform int level;

//!< out-variables
layout(location = 0) out vec4 fragColor;

void main() 
{
	vec4 thisLevel = textureLod( tex, passPosition.xy, float(level) );
	vec4 nextLevel = textureLod( tex, passPosition.xy, float(level+1));

	vec4 difference = thisLevel - nextLevel;

	fragColor = difference;
    // fragColor = difference / 2.0 + vec4(0.5);
}