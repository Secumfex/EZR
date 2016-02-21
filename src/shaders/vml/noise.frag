#version 330

#define PIXELTILESIZE 8 // an 8x8 pixel tile

in vec3 passPosition;
in vec2 passUV;

uniform int width;
uniform int height;
uniform int noise[480000];

void main() {

	int blockSize = PIXELTILESIZE * PIXELTILESIZE;

	// check if fragment should calculate sth
	vec2 pixelPosition = vec2(gl_FragCoord);

	// identify block
	vec2 dim = vec2(width, height);
	vec2 blocks = round(dim / PIXELTILESIZE) * PIXELTILESIZE;
	vec2 blockPos = round(pixelPosition / PIXELTILESIZE) * PIXELTILESIZE;
	vec2 pixelPos = mod(pixelPosition, PIXELTILESIZE);
	int blockIndex = int(blockPos.y * blocks.x + blockPos.x);
	int pixelIndex = int(pixelPos.y * PIXELTILESIZE + pixelPos.x);

	//int entry = noise[blockIndex * blockSize + pixelIndex];
	int entry = 1;

	gl_FragColor = vec4(entry, 0, 0, 1);
}
