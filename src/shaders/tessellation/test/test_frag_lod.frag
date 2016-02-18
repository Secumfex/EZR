#version 430

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragNormal;
layout(location = 2) out vec4 fragPosition;
layout(location = 3) out vec4 fragUVCoord;
layout(location = 4) out vec4 fragMaterial;

in vec3 tePosition;
in vec4 passPosition;

uniform mat4 model;
uniform mat4 view;
uniform sampler2D diff;
uniform sampler2D snow;
uniform sampler2D normal;
uniform sampler2D grass;

vec4 mixTextures(vec4 col1, vec4 col2, vec4 col3) {
	if (tePosition.y < 0.1) return col1;
	if (tePosition.y < 0.2) return mix(col1, col2, (tePosition.y - 0.1) / (0.2 - 0.1) );
	if (tePosition.y < 0.4) return col2;
	if (tePosition.y < 0.5) return mix(col2, col3, (tePosition.y - 0.4) / (0.5 - 0.4));
	else return col3;
}
void main(){
	vec4 rockColor = texture(diff, tePosition.xz * 24);
	vec4 snowColor = texture(snow, tePosition.xz * 24);
	vec4 grassColor = texture(grass, tePosition.xz *24);
	vec4 mixedColor = mixTextures(grassColor, rockColor, snowColor);

	fragColor = mixedColor;
	fragNormal = vec4(normalize((transpose(inverse(view * model )) * texture(normal, tePosition.xz)).xyz),0.0);
	fragPosition = passPosition;
	fragUVCoord = vec4(tePosition, 1.0);
	fragMaterial = vec4(0.0, 1.0, 0.2, 0.0);
}