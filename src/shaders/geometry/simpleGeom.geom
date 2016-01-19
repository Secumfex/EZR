#version 430

layout (triangles) in;
layout (triangle_strip, max_vertices=4) out;

in VertexData {
	vec2 texCoord;
	vec3 position;
	vec3 normal;
} VertexIn[];

out VertexData {
	vec2 texCoord;
	vec3 position;
	vec3 normal;
} VertexOut;

out vec2 passUVCoord;
out vec3 passPosition;
out vec3 passNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform	float strength;
// uniform float stiffness;
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

void main()
{
	// model space of triangle
	vec4 pos1 = vec4(gl_in[0].gl_Position.xyz, 1.0);
	vec4 pos2 = vec4(gl_in[1].gl_Position.xyz, 1.0);
	vec4 pos3 = vec4(gl_in[2].gl_Position.xyz, 1.0);

	// center of triangle
//	vec4 center = centerPos(pos1,pos2,pos3);
	float t = sin(1000.0 * pos1.x) + cos(500.0 * pos2.y);
	vec4 center = mix3(pos1,pos2,pos3,t);
	vec4 centerView = (view * model * center); // center in view space
	float sizeFactor = min( 1.0 / (length(centerView.xyz) * 0.5), 1.0);
	float size = sizeFactor * strength;
	vec4 upView = inverse(transpose(view * model)) * vec4(0.0,0.0, size * 2.0,0.0); // up vector in view space


	// vec2 offset in xz-plane according to wind field
	vec4 offset = (texture(vectorTexture,VertexIn[0].texCoord) * 2.0 - 1.0 ) * size;
	offset.z = offset.y;
	offset.y = 0.0;
	offset.w = 0.0;

	//bottom left
	vec4 point = vec4(-size, 0.0, 0.0, 0.0) + centerView;
	VertexOut.texCoord = vec2(0.0,0.0);
	VertexOut.position = point.xyz;
	// VertexOut.normal = vec3(0.0, 0.0, 1.0);
	VertexOut.normal = normalize(VertexIn[0].normal);
	passUVCoord = VertexOut.texCoord;
	passNormal = VertexOut.normal;
	passPosition = VertexOut.position;

	gl_Position = projection * point;
	EmitVertex();

	point = vec4(-size, 0.0, 0.0, 0.0) + upView + centerView;
	point += offset;
	VertexOut.texCoord = vec2(0.0,1.0);
	VertexOut.position = point.xyz;
	// VertexOut.normal = vec3(0.0, 0.0, 1.0);
	VertexOut.normal = normalize(VertexIn[0].normal + offset.xyz * 4.0);
	passUVCoord = VertexOut.texCoord;
	passNormal = VertexOut.normal;
	passPosition = VertexOut.position;
	gl_Position = projection * point;
	EmitVertex();
	
	point = vec4(size, 0.0, 0.0, 0.0) + centerView;
	VertexOut.texCoord = vec2(1.0,0.0);
	VertexOut.position = point.xyz;
	// VertexOut.normal = vec3(0.0, 0.0, 1.0);
	VertexOut.normal = normalize(VertexIn[0].normal);
	passUVCoord = VertexOut.texCoord;
	passNormal = VertexOut.normal;
	passPosition = VertexOut.position;
	gl_Position = projection * point;
	EmitVertex();
	
	point = vec4(size, 0.0, 0.0, 0.0) + upView + centerView;
	point += offset;
	VertexOut.texCoord = vec2(1.0,1.0);
	VertexOut.position = point.xyz;
	// VertexOut.normal = vec3(0.0, 0.0, 1.0);
	VertexOut.normal = normalize(VertexIn[0].normal + offset.xyz * 4.0);	

	passUVCoord = VertexOut.texCoord;
	passNormal = VertexOut.normal;
	passPosition = VertexOut.position;
	
	gl_Position = projection * point ;
	EmitVertex();

	EndPrimitive();
}