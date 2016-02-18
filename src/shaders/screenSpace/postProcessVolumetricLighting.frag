#version 330

/*
* Simple fragmentshader that adds a texture ontop of the other
*/

#define PI 3.14159265359

//!< in-variable
in vec3 passPosition;

//!< uniforms
uniform sampler2D tex;
uniform sampler2D addTex;

uniform float strength;
uniform float min;
uniform float max;
uniform int mode;

//!< out-variables
layout(location = 0) out vec4 fragColor;

void main() 
{
	vec4 vml = texture(addTex, passPosition.xy);
	vec4 source = texture(tex, passPosition.xy);
	float intensity = vml.x;

	switch (mode)
	{
		case 0: intensity = cos(intensity*(PI/2)); break;
		case 1: intensity = sin(intensity*(PI/2)); break;
		case 2: intensity = 1/intensity; break;
		case 3: intensity = sqrt(intensity); break;
		case 4: intensity = intensity * intensity; break;
		case 5: intensity = log(intensity+1); break;
	}

	intensity = clamp(intensity, 0.0, 1.0);

	float weight = (intensity * (max-min)) + min;

	//vec4 texColor = mix(source, vml, weight);
	vec4 texColor = (source * (1-weight)) + (vml * weight);

	//!< fragcolor gets transparency by uniform
    fragColor = texColor;
}