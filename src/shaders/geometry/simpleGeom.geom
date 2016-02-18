#version 430

layout (triangles) in;
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
uniform	float strength;
// uniform float stiffness;
uniform sampler2D vectorTexture;

#define MAX_DISTANCE 15.0
#define VARYING_SIZE_RANGE 5.0 // last 5 units

uniform sampler2D heightMap;
uniform vec4 heightMapRange; //!< x,y --> begin coords (XZ-plane) z,w --> end coords( XZ-plane )

#define HEIGHT_SCALE 17.0
#define HEIGHT_BIAS -1.5

vec2 worldToHeightMapUV(vec4 worldPos)
{
	vec2 heightMapUV;
	heightMapUV.x = (worldPos.x - heightMapRange.x) / (heightMapRange.z - heightMapRange.x );
	heightMapUV.y = (worldPos.z - heightMapRange.y) / (heightMapRange.w - heightMapRange.y);
	return heightMapUV; 
}

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

void main()
{
	// inverse transpose model view matrix
	mat4 invTransMV =  inverse(transpose(view * model));

	// model space of triangle
	vec4 pos1 = vec4(gl_in[0].gl_Position.xyz, 1.0);
	vec4 pos2 = vec4(gl_in[1].gl_Position.xyz, 1.0);
	vec4 pos3 = vec4(gl_in[2].gl_Position.xyz, 1.0);

	// center of triangle
	float t = sin(1000.0 * pos1.x) + cos(500.0 * pos2.y); //some "noise"
	vec4 centerWorld = model * mix3(pos1,pos2,pos3,t);
	
	// adjust y coord
	centerWorld.y = texture(heightMap, worldToHeightMapUV(centerWorld)).x * HEIGHT_SCALE + HEIGHT_BIAS;

	vec4 centerView = (view * centerWorld); // center in view space

	float distToCameraXZ = length(centerView.xz);
	if (distToCameraXZ > 15.0)
	{
		return;
	}
	float sizeFactor = clamp( (MAX_DISTANCE - distToCameraXZ) / VARYING_SIZE_RANGE, 0.0, 1.0);
	
	float size = sizeFactor * strength;
	
	vec4 upView = invTransMV * vec4(0.0,0.0, size * 2.0,0.0); // up vector in view space

	// vec2 offset in xz-plane according to wind field
	vec4 offset = (texture(vectorTexture,VertexGeom[0].texCoord) * 2.0 - 1.0 ) * size;
	offset.z = offset.y;
	offset.y = 0.0;
	offset.w = 0.0;

	//bottom left
	vec4 point = vec4(-size, 0.0, 0.0, 0.0) + centerView;
	passUVCoord = vec2(0.0,0.0);
	passPosition = point.xyz;
	passNormal = normalize(	( invTransMV * vec4(VertexGeom[0].normal, 0.0) ).xyz );
	passTangent = normalize(	( invTransMV * vec4(VertexGeom[0].tangent, 0.0) ).xyz );
	point.w = 1.0;
	gl_Position = projection * point;
	EmitVertex();

	point = vec4(-size, 0.0, 0.0, 0.0) + upView + centerView;
	point += offset;
	passUVCoord = vec2(0.0,1.0);
	passPosition = point.xyz;
	passNormal = normalize(	( invTransMV * vec4(VertexGeom[0].normal + offset.xyz * 4.0, 0.0) ).xyz);
	passTangent = normalize( ( invTransMV * vec4(VertexGeom[0].tangent + offset.xyz * 4.0, 0.0) ).xyz);
	point.w = 1.0;
	gl_Position = projection * point;
	EmitVertex();
	
	point = vec4(size, 0.0, 0.0, 0.0) + centerView;
	passUVCoord = vec2(1.0,0.0);
	passPosition = point.xyz;
	passNormal = normalize( ( invTransMV * vec4(VertexGeom[0].normal, 0.0) ).xyz);
	passTangent = normalize( ( invTransMV * vec4(VertexGeom[0].tangent, 0.0) ).xyz);
	point.w = 1.0;
	gl_Position = projection * point;
	EmitVertex();
	
	point = vec4(size, 0.0, 0.0, 0.0) + upView + centerView;
	point += offset;
	passUVCoord = vec2(1.0,1.0);
	passPosition = point.xyz;
	passNormal = normalize( ( invTransMV * vec4(VertexGeom[0].normal + offset.xyz * 4.0, 0.0) ).xyz);
	passTangent = normalize( ( invTransMV * vec4(VertexGeom[0].tangent + offset.xyz * 4.0, 0.0) ).xyz);
	point.w = 1.0;
	gl_Position = projection * point ;
	EmitVertex();

	EndPrimitive();
}