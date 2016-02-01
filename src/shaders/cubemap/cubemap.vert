#version 430
 
layout(location = 0) in vec4 positionAttribute;
layout(location = 1) in vec2 uvCoordAttribute;

out vec3 passTexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 pos = projection * view * vec4(positionAttribute.xyz,1);
    gl_Position = pos.xyww;

    // assuming a cube has been rendered as seen from the center
    passTexCoords = positionAttribute.xyz;
}  