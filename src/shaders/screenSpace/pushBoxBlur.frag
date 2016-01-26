#version 430

in vec2 passUV;

uniform sampler2D tex;
uniform int level;

out vec4 fragmentColor;

void main() {
	fragmentColor = vec4(0,0,0,0);

	vec4 c;

	c = texelFetch(tex, ivec2(gl_FragCoord.xy) / 2, level + 1);
	fragmentColor += c * 9.0 / 16.0;

	ivec2 sampleCoords = ivec2(gl_FragCoord.xy) / 2;

	if ( int(gl_FragCoord.x) % 2 == 0)
	{
		if ( int(gl_FragCoord.y) % 2 == 0)
		{
			c = texelFetchOffset(tex, sampleCoords, level+1, ivec2(-1,0));
			fragmentColor += c * 3.0 / 16.0;
			c = texelFetchOffset(tex, sampleCoords, level+1, ivec2(0,-1));
			fragmentColor += c * 3.0 / 16.0;
			c = texelFetchOffset(tex, sampleCoords, level+1, ivec2(-1,-1));
			fragmentColor += c * 1.0 / 16.0;
		}
		else
		{
			c = texelFetchOffset(tex, sampleCoords, level+1, ivec2(-1,0));
			fragmentColor += c * 3.0 / 16.0;
			c = texelFetchOffset(tex, sampleCoords, level+1, ivec2(0,1));
			fragmentColor += c * 3.0 / 16.0;
			c = texelFetchOffset(tex, sampleCoords, level+1, ivec2(-1,1));
			fragmentColor += c * 1.0 / 16.0;
		}
	}
	else
	{
		if ( int(gl_FragCoord.y) % 2 == 0)
		{
			c = texelFetchOffset(tex, sampleCoords, level+1, ivec2(1,0));
			fragmentColor += c * 3.0 / 16.0;
			c = texelFetchOffset(tex, sampleCoords, level+1, ivec2(0,-1));
			fragmentColor += c * 3.0 / 16.0;
			c = texelFetchOffset(tex, sampleCoords, level+1, ivec2(1,-1));
			fragmentColor += c * 1.0 / 16.0;
		}
		else
		{
			c = texelFetchOffset(tex, sampleCoords, level+1, ivec2(1,0));
			fragmentColor += c * 3.0 / 16.0;
			c = texelFetchOffset(tex, sampleCoords, level+1, ivec2(0,1));
			fragmentColor += c * 3.0 / 16.0;
			c = texelFetchOffset(tex, sampleCoords, level+1, ivec2(1,1));
			fragmentColor += c * 1.0 / 16.0;
		}
	}

}