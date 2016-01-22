#version 430

layout (points) in;
// layout (line_strip, max_vertices = 2) out;
layout (triangle_strip, max_vertices=4) out;

in VertexData {
	vec2 texCoord;
	vec3 position;
	vec3 normal;
	vec3 tangent;
} VertexGeom[];

out vec2 passUVCoord;
out vec3 passPosition;
out vec3 passNormal;
out vec3 passTangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform	float foliageSize;
uniform sampler2D vectorTexture;

float mix3(float a, float b, float c, float t)
{
	return 
		mix(
			mix(a, b, clamp(t+1,0.0,1.0)),
			c,
			clamp(t,0.0,1.0));
}
vec4 mix3(vec4 a, vec4 b, vec4 c, float t)
{
	return 
		mix(
			mix(a, b, clamp(t+1,0.0,1.0)),
			c,
			clamp(t,0.0,1.0));
}

/**
 * @brief not really the center, but whatever
 */
vec4 centerPos(vec4 pos1, vec4 pos2, vec4 pos3)
{
	vec4 dir1 = pos2 - pos1;
	vec4 dir2 = pos3 - pos2;
	return (pos1 + 0.5 * dir1 + 0.5 * dir2);
}


void main() {    

	vec4 center = vec4(VertexGeom[0].position, 1.0);

    gl_Position = projection * (center + vec4(-foliageSize, 0.0, 0.0, 0.0)); 
    // passUVCoord = VertexGeom[0].texCoord;
    passUVCoord = vec2(0,0);
    passPosition = VertexGeom[0].position;
    // passNormal = vec3(0.0,0.0,1.0);
    passNormal = normalize( VertexGeom[0].normal - vec3(foliageSize * 0.25, foliageSize * 0.25, 0.0 ));
    passTangent = VertexGeom[0].tangent;    EmitVertex();

    gl_Position = projection * (center + vec4(-foliageSize, foliageSize, 0.0, 0.0));    
    // passUVCoord = VertexGeom[0].texCoord;
    passUVCoord = vec2(0,1);
    passPosition = VertexGeom[0].position;
    // passNormal = vec3(0.0,0.0,1.0);
    passNormal = normalize( VertexGeom[0].normal + vec3(-foliageSize * 0.25, foliageSize * 0.25, 0.0));
    passTangent = VertexGeom[0].tangent;
    EmitVertex();

    gl_Position = projection * (center + vec4(foliageSize, 0.0, 0.0, 0.0));    
    passUVCoord = vec2(1,0);
    // passUVCoord = VertexGeom[0].texCoord;
    passPosition = VertexGeom[0].position;
    // passNormal = vec3(0.0,0.0,1.0);
    passNormal = normalize( VertexGeom[0].normal + vec3(foliageSize * 0.25,-foliageSize * 0.25,0.0));
    passTangent = VertexGeom[0].tangent;
    EmitVertex();
    
    gl_Position = projection * (center +vec4(foliageSize, foliageSize, 0.0, 0.0)); 
    passUVCoord = vec2(1,1);
    // passUVCoord = VertexGeom[0].texCoord;
    passPosition = VertexGeom[0].position;
    // passNormal = vec3(0.0,0.0,1.0);
    passNormal = normalize( VertexGeom[0].normal + vec3(foliageSize * 0.25, foliageSize * 0.25,0.0));
    passTangent = VertexGeom[0].tangent;
    EmitVertex();

    EndPrimitive();
} 