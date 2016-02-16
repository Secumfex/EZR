#version 430

layout(location = 0) out vec4 fragColor;
in vec3 tePosition;

uniform sampler2D diff;
uniform sampler2D terrain;
uniform sampler2D rock;

vec3 incident = normalize(vec3(1.0, 0.2, 0.5));
vec4 light = vec4(1.0, 0.95, 0.9, 1.0) * 1.1;

vec4 mixColor(vec4 col1, vec4 col2) {
	if (tePosition.y < 0.5) {
		return col1;
	}
	else if (tePosition.y > 0.6) {
		return col2;
	}
	else return mix(col1, col2, 0.5);
}

void main(){
	vec3 normal = texture(terrain, tePosition.xz).xyz;
	vec4 color = texture(diff, tePosition.xz);
	vec4 rockColor = texture(rock, tePosition.xz);
	vec4 mixedColor = mixColor(color, rockColor);
	//vec4 color = vec4(vec3(34.0f/255, 139.0f/255, 34.0f/255), 1.0);
	float dot_surface_incident = max(0, dot(normal, incident));
	color = color * light * (max(0.1, dot_surface_incident)+0.05)*1.5;
	//fragColor = mix(color, color*0.5+vec4(0.5, 0.5, 0.5, 1.0), tePosition.z*2.0);
	fragColor = color;
}
