#version 330
 
 /**
 * View aligned billboarding
 */

 //!< in-variables
layout(location = 0) in vec4 positionAttribute;
layout(location = 1) in vec2 uvCoordAttribute;
layout(location = 2) in vec4 normalAttribute;

//!< uniforms
uniform vec4 position;
uniform float scale;

uniform mat4 view;
uniform mat4 projection;

//!< out-variables
out vec3 passPosition;
out vec2 passUVCoord;


void main(){
    passUVCoord = uvCoordAttribute;

    vec4 scaledPosAttrib = vec4(scale * positionAttribute.xyz, 1.0);
    
    vec4 viewPosition = vec4(((view * position) + scaledPosAttrib).xyz, 1.0);

    passPosition = (viewPosition).xyz;

    gl_Position =  projection * viewPosition;
}
