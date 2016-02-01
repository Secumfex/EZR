#version 330
 
 /**
 * projection space billboarding
 */

 //!< in-variables
layout(location = 0) in vec4 positionAttribute;
layout(location = 1) in vec2 uvCoordAttribute;

//!< uniforms
uniform vec4 position;
uniform vec3 scale;

uniform mat4 view;
uniform mat4 projection;

//!< out-variables
out vec3 passPosition;
out vec2 passUVCoord;


void main(){
    passUVCoord = uvCoordAttribute;

    vec4 scaledPosAttrib = vec4(scale * positionAttribute.xyz, 1.0);
    
    vec4 viewPosition = view * position;
    vec4 projectionPosition = projection * viewPosition;
    projectionPosition /= projectionPosition.w;
    projectionPosition += vec4(scaledPosAttrib.xyz,0.0);

    passPosition = (viewPosition).xyz; // this isn't what you would expect
    gl_Position =  projectionPosition;
}
