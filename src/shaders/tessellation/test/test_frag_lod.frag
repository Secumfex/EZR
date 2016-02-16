#version 430

layout(location = 0) out vec4 fragColor;
in vec3 tePosition;

uniform sampler2D diff;
uniform sampler2D terrain;

vec3 incident = normalize(vec3(1.0, 0.2, 0.5));
vec4 light = vec4(1.0, 0.95, 0.9, 1.0) * 1.1;

void main(){
	vec3 normal = texture(terrain, tePosition.xz).xyz;
	vec4 color = texture(diff, tePosition.xz);
	//vec4 color = vec4(vec3(34.0f/255, 139.0f/255, 34.0f/255), 1.0);
	float dot_surface_incident = max(0, dot(normal, incident));
	color = color * light * (max(0.1, dot_surface_incident)+0.05)*1.5;
	//fragColor = mix(color, color*0.5+vec4(0.5, 0.5, 0.5, 1.0), tePosition.z*2.0);
	fragColor = color;
}
