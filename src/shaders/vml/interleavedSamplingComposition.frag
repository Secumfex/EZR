#version 330

#define PIXELTILESIZE 8 // an 8x8 pixel tile

in vec3 passPosition;
in vec2 passUV;

uniform sampler2D vliMap;

void main() {

	vec2 dim = textureSize(vliMap, 0);

	// identify block
	vec2 pixelPosition = vec2(gl_FragCoord);
    pixelPosition.xy = round(pixelPosition / PIXELTILESIZE) * PIXELTILESIZE;
    pixelPosition.xy -= 0.5;
    pixelPosition.x /= dim.x;
    pixelPosition.y /= dim.y;


	vec4 sum = vec4(0,0,0,0);
	int counter = 0;
	for (int x = 0; x < PIXELTILESIZE; x++) {
		for (int y = 0; y < PIXELTILESIZE; y++) {
			sum += textureOffset(vliMap, pixelPosition, ivec2(x, -y));
			counter++;
		}
	}

	sum /= counter;

	gl_FragColor = vec4(sum.rgb, 1.0);
}
