#version 430
 
layout(location = 0) in vec4 positionAttribute;
layout(location = 1) in vec2 uvCoordAttribute;

out vec3 passTexCoords;
out vec3 passPosition;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 pos = projection * view * vec4(positionAttribute.xyz,1);
    gl_Position = pos.xyww;

	passPosition = (view * vec4(positionAttribute.xyz,1)).xyz * 10000.0; // really far away

    // assuming a cube has been rendered as seen from the center
    passTexCoords = positionAttribute.xyz;
}  