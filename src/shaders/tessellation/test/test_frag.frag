#version 430
layout(location = 0) out vec4 fragColor;
in vec3 tePosition;

void main() {
	if (tePosition.z > 0.15) {
		fragColor = vec4(vec3(1.0), 1.0);
	}
	else {
		fragColor = vec4(vec3(0.2, 0.66, 0.2), 1.0);
	}	

fragColor = vec4(tePosition, 1.0);	
}