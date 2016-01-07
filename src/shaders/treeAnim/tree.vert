#version 430

// arbitrary size, just to make sure enough memory is allocated
#define MAX_NUM_BRANCHES 40

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
	Branch[MAX_NUM_BRANCHES] branches;
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

uniform float strength;

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
	vec4 qr;
	float half_angle = (angle * 0.5) * 3.14159 / 180.0;
	qr.x = axis.x * sin(half_angle);
	qr.y = axis.y * sin(half_angle);
	qr.z = axis.z * sin(half_angle);
	qr.w = cos(half_angle);
	return qr;
}

vec4 quatAroundY(float angle)
{
	return quatAxisAngle(vec3(0.0,1.0,0.0),angle);
}

vec3 applyQuat(vec3 v, vec4 q)
{ 
	return v + 2.0*cross(cross(v, q.xyz ) + q.w*v, q.xyz);
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

    Branch thisBranch = tree.branches[ hierarchyAttribute.x ];
    bool isTrunk = ( hierarchyAttribute.x == 0 );

    //bend vertex
    vec4 branchOrientation = bendBranch(
    	positionAttribute.xyz,
    	thisBranch.origin,
    	thisBranch.direction,
    	strength,
    	windDirection,
    	windPower);

    vec4 pos = vec4(applyQuat(positionAttribute.xyz, branchOrientation),1.0);
	
	//vec4 pos = positionAttribute;
	vec4 worldPos = (model * pos);

    passWorldPosition = worldPos.xyz;
    passPosition = (view * worldPos).xyz;
    
    gl_Position =  projection * view * model * pos;

    passWorldNormal = normalize( ( transpose( inverse( model ) ) * normalAttribute).xyz );
	passNormal = normalize( ( transpose( inverse( view * model ) ) * normalAttribute ).xyz );

	VertexOut.texCoord = passUVCoord;	
	VertexOut.normal = passNormal;
	VertexOut.position = passPosition;
}
