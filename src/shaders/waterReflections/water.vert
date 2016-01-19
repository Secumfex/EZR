#version 430

 //!< defines
 
 
//!< in-variables
in vec4 inPosition;
in vec2 inTex;		//!< umbenennen

//!< uniforms
uniform mat4 view;					//!< umbenennen
uniform mat4 worldView;				//!< umbenennen
uniform mat4 reflectionView;		//!< umbenennen
uniform mat4 worldReflectionView;	//!< umbenennen

//!< out-variables
out vec4 samplPosition;
out vec4 reflectionMapSamplPos;
out vec2 bumpMapSamplPos;
out vec4 refractionMapSamplPos;
out vec4 position3D;

void main(){

//!< ???
samplPosition = inPosition * worldView;
reflectionMapSamplPos = inPosition * worldReflectionView;
refractionMapSamplPos = inPosition * worldView;

//!< ???
vec2 moveVec = vec2(0,1);
bumpMapSamplPos = inTex / waveLenght + time * windForce * moveVec;

}



//!< alternativ

varying vec4 waterTex0;
varying vec4 waterTex1;
varying vec4 waterTex2;
varying vec4 waterTex3;
varying vec4 waterTex4;

uniform vec4 viewpos, lightpos;
uniform float time, time2;

//unit 0 = water_reflection
//unit 1 = water_refraction
//unit 2 = water_normalmap
//unit 3 = water_dudvmap
//unit 4 = water_depthmap

void main(void)
{
	vec4 mpos, temp;
    vec4 tangent = vec4(1.0, 0.0, 0.0, 0.0);
    vec4 norm = vec4(0.0, 1.0, 0.0, 0.0);
    vec4 binormal = vec4(0.0, 0.0, 1.0, 0.0);

    mat4 mvp = gl_ModelViewProjectionMatrix;
    mat4 mtx = gl_TextureMatrix[0];

    temp = viewpos - gl_Vertex;

    waterTex4.x = dot(temp, tangent);
    waterTex4.y = dot(temp, binormal);
    waterTex4.z = dot(temp, norm);
    waterTex4.w = 0.0;

    temp = lightpos - gl_Vertex;

    waterTex0.x = dot(temp, tangent);
    waterTex0.y = dot(temp, binormal);
    waterTex0.z = dot(temp, norm);
    waterTex0.w = 0.0;

    mpos = mvp * gl_Vertex;

    vec4 t1 = vec4(0.0, -time, 0.0,0.0);
    vec4 t2 = vec4(0.0, -time2, 0.0,0.0);
    
    waterTex1 = gl_MultiTexCoord0 + t1;
    waterTex2 = gl_MultiTexCoord0 + t2;
    waterTex3 = mpos;

    gl_Position = ftransform();

}