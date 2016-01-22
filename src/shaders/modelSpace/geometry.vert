#version 330
 
 /**
 * This simple shader passes out all important Attributes without computing anything.
 * Use this when all computation is done in a Geometry Shader
 */

 //!< in-variables
layout(location = 0) in vec4 positionAttribute;
layout(location = 1) in vec2 uvCoordAttribute;
layout(location = 2) in vec4 normalAttribute;
layout(location = 3) in vec4 tangentAttribute;

//!< out-variables
out VertexData {
	vec2 texCoord;
	vec3 position;
	vec3 normal;
	vec3 tangent;
} VertexGeom;

void main(){
    gl_Position = positionAttribute;
    VertexGeom.texCoord = uvCoordAttribute;
    VertexGeom.position = positionAttribute.xyz;
    VertexGeom.normal = normalAttribute.xyz;
    VertexGeom.tangent = tangentAttribute.xyz;
}
