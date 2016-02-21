#version 430

//incoming data for the single textures
in vec3 passPosition;
in vec2 passUVCoord;
in vec3 passNormal;


uniform vec4  color;
uniform float mixTexture;
uniform sampler2D tex;

uniform vec2 texResolution;
uniform int peel_level;
uniform sampler2D lastDepth;

//writable textures for deferred screen space calculations
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragNormal;
layout(location = 2) out vec4 fragPosition;
layout(location = 3) out vec4 fragUVCoord;
 
void main(){

	fragColor = color;
	if ( mixTexture != 0.0)
	{
		fragColor = mix(color, texture(tex, passUVCoord), mixTexture );
	}

	// check with before depth peel from lastDepth texture
	if (peel_level > 0)
	{
		float depth = texture(lastDepth, gl_FragCoord.xy / texResolution).r;

		if ( gl_FragCoord.z <= depth.r)
		{
			discard;
		}
	}

    fragPosition = vec4(passPosition,1);
    fragUVCoord = vec4(passUVCoord,0,0);
    fragNormal = vec4(passNormal,0);
}
