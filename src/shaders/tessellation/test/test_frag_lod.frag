#version 430

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragNormal;
layout(location = 2) out vec4 fragPosition;
layout(location = 3) out vec4 fragUVCoord;
layout(location = 4) out vec4 fragMaterial;

in vec3 tePosition;
in vec4 passPosition;

uniform sampler2D diff;
uniform sampler2D normal;
uniform sampler2D snow;

vec4 mixColor(vec4 col1, vec4 col2) {
	if (tePosition.y < 0.5) {
		return col1;
	}
	else if (tePosition.y > 0.6) {
		return col2;
	}
	else return mix(col1, col2, tePosition.y);
}

void main(){
	vec4 color = texture(diff, tePosition.xz * 24);
	vec4 snowColor = texture(snow, tePosition.xz * 24);
	vec4 mixedColor = mixColor(color, snowColor);

	fragColor = mixedColor;
	fragNormal = texture(normal, tePosition.xz);
	//fragNormal = vec4(0.0, 1.0, 0.0, 1.0);
	//fragPosition = vec4(tePosition, 1.0);
	fragPosition = passPosition;
	fragUVCoord = vec4(tePosition, 1.0);
	fragMaterial = vec4(0.0, 1.0, 0.2, 0.0);
}
