#version 430

//incoming data for the single textures
in vec3 passPosition;
in vec2 passUVCoord;
in vec3 passNormal;
in vec3 passTangent;

uniform float time;

uniform vec4  color;
uniform float mixTexture;
uniform sampler2D tex;

uniform bool hasNormalTex;
uniform sampler2D normalTex;

uniform float materialType; // 0 : usual phong // 1: no lighting // 2: reflectant 
uniform float shininess;    // exponent of specular highlight (reflectancy)
uniform float shininess_strength; // strength of specular highlight

//writable textures for deferred screen space calculations
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragNormal;
layout(location = 2) out vec4 fragPosition;
layout(location = 3) out vec4 fragUVCoord;
layout(location = 4) out vec4 fragMaterial;
 
void main(){
	fragColor = color;
	vec3 normalView = passNormal;

	vec2 scaled_uv = passUVCoord * 2.5;
	vec2 wabbling_uv_1 = vec2(scaled_uv.x + sin(time), scaled_uv.y);
	vec2 wabbling_uv_2 = vec2(scaled_uv.x, scaled_uv.y + cos(time));
	
	//vec4 wabbling_normal_1 = texture(normalTex, wabbling_uv_1);
	//vec4 wabbling_normal_2 = texture(normalTex, wabbling_uv_2);
	//vec4 wabbling_result_normal = normalize((wabbling_normal_1 + wabbling_normal_2));
	
	
	if ( mixTexture != 0.0)
	{
		fragColor = mix(color, texture(tex, passUVCoord), mixTexture );
	}

	if (hasNormalTex)
	{
		vec3 nrm = normalize(passNormal);
		vec3 tan = normalize(passTangent);
		vec3 binormalView = normalize(cross(nrm, tan));
		mat3 tangentSpaceView = mat3(
			tan.x,  tan.y,   tan.z,  // first column
			binormalView.x, binormalView.y,  binormalView.z, // second column
			nrm.x,   nrm.y,     nrm.z   // third column
		);
		//vec3 normalTangentSpace = 2.0 * (texture(normalTex, passUVCoord).xyz) - 1.0;
		
		///// problem zone /////
		vec3 normalTangentSpace_1 = (2.0 * texture(normalTex, wabbling_uv_1).xyz) - 1.0;
		vec3 normalTangentSpace_2 = (2.0 * texture(normalTex, wabbling_uv_2).xyz) - 1.0;
		vec3 normalTangentSpace = normalize(normalTangentSpace_1 + normalTangentSpace_2);
		///////////
		
		normalView = tangentSpaceView * normalTangentSpace;


	}
	fragPosition = vec4(passPosition,1);
	fragUVCoord = vec4(passUVCoord,0,0);
	fragNormal = vec4(normalView,0);
	fragMaterial = vec4(materialType, shininess, shininess_strength, 0);
}