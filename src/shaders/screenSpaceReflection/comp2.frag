#version 430

//!< in-vars
in vec2 vert_UV;

//!< out-vars
out vec4 FragColor;

uniform float camNearPlane;
uniform float camFarPlane;

uniform int textureID;

uniform sampler2D vsPositionTex;
uniform sampler2D vsNormalTex;
uniform sampler2D ColorTex;
uniform sampler2D ReflectanceTex;
uniform sampler2D MaterialTex;
uniform sampler2D DepthTex;
uniform sampler2D DiffuseTex;
uniform sampler2D SSRTex;

//!< funcs
// Linearizes a depth value
float linearizeDepth(float depth){
	return (2.0f * camNearPlane) / (camFarPlane + camNearPlane - depth * (camFarPlane - camNearPlane));
}

//!< main
void main(void){
	vec4 diffuse      = texture(DiffuseTex, vert_UV);
	//*** Reflections ***
	float Reflectance = texture(ReflectanceTex, vert_UV).a;

	vec4 SSR          = vec4(texture(SSRTex, vert_UV).rgb, 1.0);

	// Compositing
	if(textureID == 0){
		FragColor = diffuse + SSR;	//+ BB + EnvMap
	}
	// View space positions
	else if(textureID == 1){
		FragColor = texture(vsPositionTex, vert_UV);
	}
	// View space normals
	else if(textureID == 2){
		FragColor = texture(vsNormalTex, vert_UV);
	}
	// Albedo (color)
	else if(textureID == 3){
		FragColor = texture(ColorTex, vert_UV);
	}
	// Material IDs
	else if(textureID == 4){
		float id = texture(MaterialTex, vert_UV).a;
		if(id == 0.00){
			FragColor = vec4(1.0);
		}
		else if(id <= 0.01){
			FragColor = vec4(1.0, 0.0, 0.0, 1.0);
		}
		else if(id <= 0.02){
			FragColor = vec4(0.0, 1.0, 0.0, 1.0);
		}
		else if(id <= 0.03){
			FragColor = vec4(0.0, 0.0, 1.0, 1.0);
		}
		else if(id <= 0.04){
			FragColor = vec4(1.0, 1.0, 0.0, 1.0);
		}
		else if(id <= 0.05){
			FragColor = vec4(1.0, 0.0, 1.0, 1.0);
		}
		else if(id <= 0.06){
			FragColor = vec4(0.0, 1.0, 1.0, 1.0);
		}
		else if(id <= 0.07){
			FragColor = vec4(1.0, 0.5, 0.5, 1.0);
		}
		else if(id <= 0.08){
			FragColor = vec4(0.0, 0.5, 1.0, 1.0);
		}
		else if(id <= 0.09){
			FragColor = vec4(1.0, 0.0, 0.5, 1.0);
		}
		else if(id <= 0.10){
			FragColor = vec4(0.25, 0.5, 0.25, 1.0);
		}
	}
	// Depth
	else if(textureID == 5){
		float linearDepth = linearizeDepth(texture(DepthTex, vert_UV).z);
		FragColor = vec4(vec3(linearDepth), 1.0);
	}
	// Reflectance
	else if(textureID == 6){
		FragColor = vec4( texture(ReflectanceTex, vert_UV).a );
	}
	// Screen space reflections
	else if(textureID == 7){
		vec3 color = texture(SSRTex, vert_UV).rgb;
		FragColor = vec4(color, 1.0);
	}
	else if(textureID == 8){
	vec3 color = texture(DiffuseTex, vert_UV).rgb;
	FragColor = vec4(color, 1.0);
	}
}