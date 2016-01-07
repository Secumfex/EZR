#version 430

// arbitrary size, just to make sure enough memory is allocated
#define MAX_NUM_BRANCHES 30

struct Branch
{
	vec3  origin;
	vec3  direction;
	vec3  tangent;
	float stiffness;
	int   parentIdx;
};

struct Tree 
{
	Branch[20] branches;
};

 //!< in-variables
layout(location = 0) in vec4 positionAttribute;
layout(location = 1) in vec2 uvCoordAttribute;
layout(location = 2) in vec4 normalAttribute;
layout(location = 4) in uvec3 hierarchyAttribute;//!< contains the indices of this and the parent branches
 //layout(location = 5) in vec2 branchWeights; //!< ToDo: should contain the weights of the vertex' branch and the parent's branch

//!< uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float simTime; //!< used for noise function
uniform float simTimeWithDelay; //!< used for phase shift or sth?
uniform float branchMovementRandomization; //!< used for randomization

uniform float delayedWindPower;
uniform vec3  windDirection; // global wind direction
uniform float windPower;

uniform float branchSuppressPower; //!< some paramete rdescribing the power of being suppressed by front facing wind (?)
uniform float branchSwayPowerA; //!< some parameter describing the power of swaying due to front facing wind (?)
uniform float branchSwayPowerB; //!< some parameter describing the power of swaying due to being perpendicular to wind direction (?)

uniform Tree tree; //!< the whole tree hierarchy

//!< out-variables
out vec3 passWorldPosition;
out vec3 passPosition;
out vec2 passUVCoord;
out vec3 passWorldNormal;
out vec3 passNormal;

out VertexData {
	vec2 texCoord;
	vec3 position;
	vec3 normal;
} VertexOut;


vec4 quatAxisAngle(vec3 axis, float angle)
{
	//TODO convert angle axis to quaternion
	vec4 result;
	return result;
}

vec4 quatAroundY(float angle)
{
	//TODO convert angle around Y-Axis to quaternion
	//TODO make sure Y-Axis is correct
	vec4 result;
	return result;
}

vec4 applyQuat(vec4 vector, vec4 quaternion)
{
	vec4 result;
	//TODO quat to Rotation-Matrix, apply Matrix
	return result;
}

vec4 bendBranch(vec3 pos,  
                 vec3 branchOrigin,  
                 vec3 branchUp,  
                 float  branchNoise,  
                 vec3 windDir,  
                 float  windPow)  
{  
  vec3 posInBranchSpace = pos - branchOrigin.xyz;  
  float towardsX = dot(normalize(vec3(posInBranchSpace.x, 0,  
                                        posInBranchSpace.z)),  
                       vec3(1, 0, 0));  
  float facingWind = dot(normalize(vec3(posInBranchSpace.x, 0,  
                                          posInBranchSpace.z)),  
                         windDir);  
  float a = branchSwayPowerA * cos(simTime + branchNoise *  
                                   branchMovementRandomization);  
  float b = branchSwayPowerB * cos(simTimeWithDelay + branchNoise *  
                                   branchMovementRandomization);  
  float oldA = a;  
  a = -0.5 * a + branchSuppressPower * branchSwayPowerA;  
  b *= windPower;  
  a = mix(oldA * windPow, a * windPow, delayedWindPower *  
           clamp(1 - facingWind, 0.0, 1.0));  
  vec3 windTangent = vec3(-windDir.z, windDir.y, windDir.x);  
  vec4 rotation1 = quatAxisAngle(windTangent, a);  
  vec4 rotation2 = quatAroundY(b);  
  return mix(rotation1, rotation2, 1 - abs(facingWind));  
} 

void main(){
    passUVCoord = uvCoordAttribute;

    //Branch thisBranch = tree.branches[ hierarchyAttribute[0] ];
    //bool isTrunk = ( hierarchyAttribute[0] == 0 );

    //bend vertex
    //vec4 branchOrientation = bendBranch(
    //	positionAttribute.xyz,
    //	thisBranch.origin,
    //	thisBranch.direction,
    //	1.0,
    //	windDirection,
    //	windPower);

    //vec4 pos = applyQuat(positionAttribute, branchOrientation);

	// for testing purposes
	float h0 = float(hierarchyAttribute.x);
	float h1 = float(hierarchyAttribute.y);
	float h2 = float(hierarchyAttribute.z);
	vec4 pos = vec4( h0, h1, h2, 1.0);
	vec4 worldPos = (model * pos);

    passWorldPosition = worldPos.xyz;
    passPosition = (view * worldPos).xyz;
    
    //gl_Position =  projection * view * model * positionAttribute;
    gl_Position =  projection * view * model * pos;


    passWorldNormal = normalize( ( transpose( inverse( model ) ) * normalAttribute).xyz );
	passNormal = normalize( ( transpose( inverse( view * model ) ) * normalAttribute ).xyz );

	VertexOut.texCoord = passUVCoord;	
	VertexOut.normal = passNormal;
	VertexOut.position = passPosition;
}
