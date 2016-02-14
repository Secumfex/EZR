#version 330
in vec3 passTexCoords;
in vec3 passPosition;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 normal;
layout(location = 2) out vec4 position;
layout(location = 3) out vec4 fragUVCoord;

uniform samplerCube skybox;

void main()
{    
    color = texture(skybox, passTexCoords);
	position = vec4(passPosition, 1.0);
	normal = vec4(normalize(-passPosition),0);
	fragUVCoord = vec4(passTexCoords,0);
}