#version 430
layout(triangles, equal_spacing, cw) in;

in vec3 tcPosition[];

out vec3 tePosition;
out vec3 tePatchDistance;

uniform mat4 projection;
uniform mat4 model;

void main()
{
	vec3 A = tePosition[2] - tePosition[0];
   	vec3 B = tePosition[1] - tePosition[0];

    vec3 p0 = gl_TessCoord.x * tcPosition[0];
    vec3 p1 = gl_TessCoord.y * tcPosition[1];
    vec3 p2 = gl_TessCoord.z * tcPosition[2];
    tePatchDistance = gl_TessCoord;
    tePosition = normalize(p0 + p1 + p2);
    gl_Position = projection * model * vec4(tePosition, 1);
}
