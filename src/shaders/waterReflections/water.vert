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