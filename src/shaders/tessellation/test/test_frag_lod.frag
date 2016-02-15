#version 430

layout(location = 0) out vec4 fragColor;
in vec3 tePosition;

//in vec2 texcoord;
//in float depth;

//out vec4 fragment;

//uniform sampler2D diffuse;
//uniform sampler2D terrain;
//uniform sampler2D noise_tile;

vec3 incident = normalize(vec3(1.0, 0.2, 0.5));
vec4 light = vec4(1.0, 0.95, 0.9, 1.0) * 1.1;

void main(){
	//vec3 normal = normalize(texture(terrain, tePosition.xy).xyz);
	vec3 normal = vec3(0.0, 1.0, 0.0);
	//vec3 normal = tePosition;
	//vec4 color = texture(diffuse, texcoord);
	vec4 color = vec4(vec3(34.0f/255, 139.0f/255, 34.0f/255), 1.0);
	//float noise_factor = texture(noise_tile, texcoord*32).r+0.1;
	float dot_surface_incident = max(0, dot(normal, incident));
	color = color * light * (max(0.1, dot_surface_incident)+0.05)*1.5;
	fragColor = mix(color, color*0.5+vec4(0.5, 0.5, 0.5, 1.0), tePosition.z*2.0);
}
