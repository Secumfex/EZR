#version 330

/*
* This shader renders something on a screenfilling quad. 
* The renderpass needs a quad as VAO.
*/

//!< in-variables
in vec4 pos;

uniform vec4 lightData;
uniform vec2 invTexRes;

//!< out-variables
out vec2 passSampleCoord;

void main() {
	//adjust depending on the resolution of your depth buffer!
	vec2 pixel = invTexRes ;

	//the quad will take depth samples in a 16x16px region of the depth buffer, centred around the light position
	passSampleCoord = lightData.xy + (pos.xy * 2 - 1) * (pixel * 8);
	gl_Position = vec4(pos.xy * 2 - 1, 0, 1);
}
