//!< light fragment shader
#version 430 core

//!< in-vars
in vec2 vert_UV;

//!< out-vars
layout (location = 0) out vec4 FragColor;

//!< uniforms
//lightInfo
uniform vec3 lightPosition;		//[30] entfernt, ebenso [i] bei zugriffen
uniform vec3 lightDiffuse;		//[30] entfernt, ebenso [i] bei zugriffen
uniform vec3 lightSpecular;
uniform int lightCount;

//uniform int textureID;
uniform int shadingModelID;
uniform float Shininess;
//uniform float Attenu;
uniform vec3 ambientColor;
uniform mat4 view;
//uniform mat4 projection;	//not used!

uniform sampler2D vsPositionTex;
uniform sampler2D vsNormalTex;
uniform sampler2D ColorTex;

//!< funcs
// Diffuse shading function
vec2 diffuseCooef(in float dot ){
  // Front facing light
  if(dot > 0)
    return vec2(dot, 1.0f);
  else
    return vec2(0.0f, 0.0f);
}

// Specular shading function
float specularCooef(in float dot, in float shininess){
	// Front facing light
	if(dot > 0)
		return pow(dot, shininess);
	else
		return 0;
}

// Attenuate a light source 
float attenuation(in vec3 vsLightPosition, in vec3 vsPosition, in float attenuationFactor){
	vec4 lightAttenuation = vec4(attenuationFactor);

	float powers = pow(vsLightPosition.x - vsPosition.x, 2.0) + pow(vsLightPosition.y - vsPosition.y, 2.0) + pow(vsLightPosition.z - vsPosition.z, 2.0); 
	float Attenuation = lightAttenuation.y + lightAttenuation.z * sqrt(powers) + lightAttenuation.w * powers;
	return 1.0/Attenuation;
}

// Phong shading
vec3 phongShading(in vec3 lightVector, in vec3 normal, in vec3 materialColor, in vec3 lightColorD, in vec3 lightColorS, in float shininess){	
	vec3 shaded = vec3(0.0f);

	vec2 cD = diffuseCooef(dot(normal, lightVector));
	float cS = specularCooef(dot(normal, lightVector), shininess);

	// Diffuse term
	shaded += lightColorD * materialColor * cD.x; // * Kd;
	// Specular term
	shaded += lightColorS * materialColor * cS; // * Ks; 

	return shaded;
}

// Cook-Torrance Shading
vec3 cooktorranceShading(in vec3 vsLightPosition, in vec3 vsPosition, in vec3 lightVector, in vec3 halfVector, in vec3 normal, in vec3 materialColorD, in vec3 lightColorD, in vec3 lightColorS, in float shininess){
	vec3 shaded = vec3(0.0f);

	// Variables
	float roughness = 0.08f;
    float refractionIndex = 1.45;

    // Beckman's distribution function D
    float normalDotHalf = dot(normal, halfVector);
    float normalDotHalfExp = normalDotHalf * normalDotHalf;
    float roughnessExp = roughness * roughness;
    float exponent = -(1.0 - normalDotHalfExp) / (normalDotHalfExp * roughnessExp);
    float e = 2.71828182845904523536028747135;
    float D = pow(e, exponent) / (roughnessExp * normalDotHalfExp * normalDotHalfExp);

    // Compute Fresnel term F
    float normalDotEye = dot(normal, vsPosition);
    float F = mix(pow(1.0 - normalDotEye, 0.15), 0.5, refractionIndex);

     // Compute self shadowing term G
    float normalDotLight = dot(normal, lightVector);
    float X = 2.0 * normalDotHalf * dot(vsPosition, halfVector);
    float G = min(0.5, min(X * normalDotLight, X * normalDotEye));

    // Compute final Cook-Torrance specular term, load textures, mix them
    float pi = 3.1415926535897932384626433832;
    float CookTorrance = (D * G * F) / (normalDotEye * pi);

    vec4 color = texture2D(ColorTex, vert_UV);
    vec4 diffuse = color * max(0.0, normalDotLight) * vec4(lightColorD, 1.0);
    vec4 specular = color * max(0.0, CookTorrance) * vec4(lightColorS, 1.0);


    shaded = vec3(diffuse + specular);

	return shaded;
}

//!< main
void main(void){	
	//*** Texture information from G-Buffer ***
	vec4 vsPosition = texture(vsPositionTex, vert_UV);
	vec3 vsNormal = texture(vsNormalTex, vert_UV).xyz;
	vec4 materialColorD = texture(ColorTex, vert_UV);

	//*** Shading ***
	// Ambient term
	vec4 shaded = vec4(ambientColor, 1.0f);

	// Lights are shaded only with light color
	// Everything else is shaded with diffuse shading	
	vec3 vsLightPosition = vec3(0.0);
	vec3 lightVector;
	vec3 halfVector;
	
	//*** Correct lighting ***
	// Perform shading for every light source
	for(int i=0; i < lightCount; i++){	
		if(materialColorD.a == 0.00 || materialColorD.a == 0.99){
			shaded = vec4(materialColorD.rgb, 1.0);
		}
		else{
			lightVector = normalize( (view *  vec4(lightPosition, 1.0f)) - vsPosition ).xyz;
			halfVector = normalize(lightVector);
			vsLightPosition = vec3(view * vec4(lightPosition, 1.0f));

			float Luminosity = attenuation(vsLightPosition, vsPosition.xyz, 0.025f);

			vec3 lightDiffuse = lightDiffuse.rgb;
			vec3 lightSpecular = lightSpecular.rgb;

			if(shadingModelID == 0)
				shaded += vec4( phongShading(lightVector, vsNormal, materialColorD.rgb, lightDiffuse, lightSpecular , Shininess), 1.0f ) * Luminosity;
			else if(shadingModelID == 1)
				shaded += vec4( cooktorranceShading(vsLightPosition, vsPosition.xyz, lightVector, halfVector, vsNormal, materialColorD.rgb, lightDiffuse, lightSpecular , Shininess), 1.0f ) * Luminosity;
		}
	}
	FragColor = shaded;
}