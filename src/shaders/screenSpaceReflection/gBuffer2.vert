//gBuffer vertex shader
#version 430
 
 //in-vars
layout(location = 0) in vec4 positionAttribute;
layout(location = 1) in vec2 uvCoordAttribute;
layout(location = 2) in vec4 normalAttribute;
layout(location = 3) in vec4 tangentAttribute;

//out-vars
out vec3 passWorldPosition;
out vec3 passPosition;
out vec2 passUVCoord;
out vec3 passWorldNormal;
out vec3 passNormal;
out vec3 passWorldTangent;
out vec3 passTangent;

//uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

//main
void main(){
    passUVCoord = uvCoordAttribute;
    vec4 worldPos = (model * positionAttribute);

    passWorldPosition = worldPos.xyz;
    passPosition = (view * worldPos).xyz;
    
    gl_Position =  projection * view * model * positionAttribute;

	mat4 normalMatWorld = transpose( inverse( model ) );
	mat4 normalMat      = transpose( inverse( view * model ) );
    passWorldNormal = normalize( (normalMatWorld * normalAttribute).xyz);
	passNormal = normalize( (normalMat * normalAttribute ).xyz );

    passWorldTangent = normalize( (normalMatWorld * tangentAttribute).xyz);
	passTangent = normalize( (normalMat * tangentAttribute ).xyz);
}
