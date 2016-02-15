//!< gbuffer fragment shader
#version 430

//!< in-vars
in vec3 vert_wsPosition;
in vec3 vert_wsNormal;
in vec3 vert_vsPosition;
in vec3 vert_vsNormal;
in vec2 vert_UV;
in vec3 vert_vsEyeVector;
in vec3 vert_wsEyePosition;
in vec3 vert_wsEyeVector;

//!< out-vars
//nach guido
	//layout (location = 0) out vec3 wsPosition;
//layout (location = 0) out vec3 vsPosition;
//layout (location = 1) out vec3 vsNormal;
//layout (location = 2) out vec4 Color;
	//layout (location = 3) out vec3 wsNormal;
//layout (location = 3) out vec4 Reflectance;
	// Location 8 is DepthBuffer
//nach arend
layout(location = 0) out vec4 Color;
layout(location = 1) out vec3 vsNormal;		//vec4
layout(location = 2) out vec3 vsPosition;	//vec4
layout(location = 3) out vec4 Reflectance;

//!< uniforms
// Material informations
uniform float matId;
//uniform float matReflectance;

uniform bool useNormalMapping;
uniform vec3 lightColor;
uniform mat4 wsNormalMatrix;
//uniform mat4 vsNormalMatrix;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
//uniform mat4 MVPMatrix;	//not used

uniform sampler2D ColorTex;
//uniform sampler2D ImpostorTex[3];
uniform sampler2D NormalTex;
//uniform samplerCube CubeMapTex;

//!< fresnel func
float fresnel(vec3 reflection, vec3 normal, float R0) {
    // float const R0 =  pow(1.0 - refractionIndexRatio, 2.0) /
    //                   pow(1.0 + refractionIndexRatio, 2.0);
    // reflection and normal are assumed to be normalized
    return R0 + (1.0 - R0) * pow(1.0 - dot(reflection, normal), 5.0);
}

//!< gauss blur y func
vec4 FastGaussianBlurY(in sampler2D texture, in vec2 texCoord){
	float blurSize = 1.0/(800.0);
	vec4 sum = vec4(0.0);
 	
 	int lod = 4;

   // blur in y (vertical)
   // take nine samples, with the distance blurSize between them
   sum += textureLod(texture, vec2(texCoord.x - blurSize, texCoord.y), lod) * 0.05;
   sum += textureLod(texture, vec2(texCoord.x - blurSize, texCoord.y), lod) * 0.09;
   sum += textureLod(texture, vec2(texCoord.x - blurSize, texCoord.y), lod) * 0.12;
   sum += textureLod(texture, vec2(texCoord.x - blurSize, texCoord.y), lod) * 0.15;
   sum += textureLod(texture, vec2(texCoord.x, texCoord.y), lod) * 0.16;
   sum += textureLod(texture, vec2(texCoord.x + blurSize, texCoord.y), lod) * 0.15;
   sum += textureLod(texture, vec2(texCoord.x + blurSize, texCoord.y), lod) * 0.12;
   sum += textureLod(texture, vec2(texCoord.x + blurSize, texCoord.y), lod) * 0.09;
   sum += textureLod(texture, vec2(texCoord.x + blurSize, texCoord.y), lod) * 0.05;
 
   return sum;
}

//!< gauss blur x func
vec4 FastGaussianBlurX(in sampler2D texture, in vec2 texCoord){
	float blurSize = 1.0/(800.0);

	vec4 sum = vec4(0.0);

 	int lod = 4;
	
	// blur in y (vertical)
	// take nine samples, with the distance blurSize between them
	sum += textureLod(texture, vec2(texCoord.x, texCoord.y - blurSize), lod) * 0.05;
	sum += textureLod(texture, vec2(texCoord.x, texCoord.y - blurSize), lod) * 0.09;
	sum += textureLod(texture, vec2(texCoord.x, texCoord.y - blurSize), lod) * 0.12;
	sum += textureLod(texture, vec2(texCoord.x, texCoord.y - blurSize), lod) * 0.15;
	sum += textureLod(texture, vec2(texCoord.x, texCoord.y), lod) * 0.16;
	sum += textureLod(texture, vec2(texCoord.x, texCoord.y + blurSize), lod) * 0.15;
	sum += textureLod(texture, vec2(texCoord.x, texCoord.y + blurSize), lod) * 0.12;
	sum += textureLod(texture, vec2(texCoord.x, texCoord.y + blurSize), lod) * 0.09;
	sum += textureLod(texture, vec2(texCoord.x, texCoord.y + blurSize), lod) * 0.05;
 
   return sum;
}

// Normal mapping: calculate cotangents
// @source: http://www.thetenthplanet.de/archives/1180
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv){
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );
 
    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
}

// Normal mapping: 
// @source: http://www.geeks3d.com/20130122/normal-mapping-without-precomputed-tangent-space-vectors/
vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord ){
    // assume N, the interpolated vertex normal and 
    // V, the view vector (vertex to eye)
    vec3 map = texture(NormalTex, vert_UV).xyz;
    map = map * 255./127. - 128./127.;
    mat3 TBN = cotangent_frame(N, V, vert_UV);
    return normalize(TBN * map);
}

//!< main
void main(void){
	//fill gbuffer
	//pos
	vec3 wsPosition = vert_wsPosition;
	vsPosition = vert_vsPosition;
	//normals
	vsNormal = normalize(vert_vsNormal);
	vec3 wsNormal = normalize(vert_wsNormal);
	// Colors (Albedo) & Alpha blending
	float alpha = texture(ColorTex, vert_UV).a;
	if(alpha < 0.75){
		discard;
	}
	vec3 OutputColor = texture(ColorTex, vert_UV).rgb * alpha;
	Color.a = alpha;
	Color.rgb = OutputColor;
	//reflectance
	Reflectance.a = 0.0;
	Reflectance.a = texture(NormalTex, vert_UV).a;
	//materials
	Color.a = matId;
	if(matId == 0.99){
		Color.rgb = vec3(1.0, 0.0, 0.0);
		Color.a = 0.09;
	}
	//lights
	if(matId == 0.00){
		Color.rgb = lightColor;
		Reflectance.a = 0.0;
	}
	//eye vectors
	vec3 wsEyeVec = normalize(vert_wsEyeVector); 
	vec3 vsEyeVec = normalize(vert_vsEyeVector);
	
	//normal mapping
	vec3 normalmap = texture(NormalTex, vert_UV).rgb;
	if(useNormalMapping){
		if(normalmap.r != 0 && normalmap.g != 0 && normalmap.b != 0){
			vec3 vsN = vsNormal;
		  	vec3 vsV = normalize(vert_vsEyeVector.xyz);
		  	vec3 vsPN = perturb_normal(vsN, vsV, vert_UV);
			vsNormal = vsPN;
			
			vec3 wsN = wsNormal;
		  	vec3 wsV = normalize(vert_wsEyeVector.xyz);
		  	vec3 wsPN = perturb_normal(wsN, wsV, vert_UV);
			wsNormal = wsPN;
		}
	}

	//reflection vectors
	vec3 wsReflectVec = normalize( reflect(wsEyeVec, wsNormal) );
	vec3 vsReflectVec = normalize( reflect(vsEyeVec, vsNormal) ); 

	float f = fresnel(wsReflectVec, wsNormal, 0.15); 
}