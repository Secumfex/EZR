#version 430

// arbitrary size, just to make sure enough memory is allocated
#define MAX_NUM_BRANCHES 40
#define EPSILON 0.0000001
#define PI 3.1415926535897932384626433832795

struct Branch
{
	vec3  origin;
	vec4  orientation;
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
	float half_angle = (angle * 0.5) * PI / 180.0;
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

vec4 multQuat(vec4 q, vec4 p) // p first, then q
{
	vec4 result;
	result.w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;
	result.x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
	result.y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
	result.z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
	return result;
}


vec4 rotation(vec3 orig, vec3 dest)
{
	float cosTheta = dot(orig, dest);
	vec3 rotationAxis;

	if(cosTheta < (-1.0 + EPSILON))
	{
		rotationAxis = cross(vec3(0.0, 0.0, 1.0), orig);
		if(length(rotationAxis) < EPSILON) // bad luck, they were parallel, try again!
		{
			rotationAxis = cross(vec3(1.0, 0.0, 0.0), orig);
		}

		rotationAxis = normalize(rotationAxis);
		return quatAxisAngle(rotationAxis, PI * 180.0);
	}

	// Implementation from Stan Melax's Game Programming Gems 1 article
	rotationAxis = cross(orig, dest);

	float s = sqrt((1.0 + cosTheta) * 2.0);
	float invs = 1.0 / s;

	return vec4(
		s * 0.5, 
		rotationAxis.x * invs,
		rotationAxis.y * invs,
		rotationAxis.z * invs);
}

vec4 bendBranch( vec3 pos,  
                 vec3 branchOrigin,  
                 vec3 branchUp,  
                 float  branchNoise,  
                 vec3 windDir,  
                 float  windPow)  
{  
	vec3 posInBranchSpace = pos - branchOrigin.xyz;  

	float towardsX = dot(normalize(vec3(posInBranchSpace.x, 0, posInBranchSpace.z)), vec3(1, 0, 0));  
	
	float facingWind = dot(normalize(vec3(posInBranchSpace.x, 0, posInBranchSpace.z)), windDir);  
	
	float a = branchSwayPowerA * cos(simTime + branchNoise * branchMovementRandomization);          // amp 1 (around branch origin)
	float b = branchSwayPowerB * cos(simTimeWithDelay + branchNoise * branchMovementRandomization); // amp 2 (around Y-axis)
	
	float oldA = a; // save temp (full sway power in wind)
	a = -0.5 * a + branchSuppressPower * branchSwayPowerA; // suppress amp1 (reduced sway power in wind)
	
	// final amplitudes, dependent on actual current wind power
	b *= windPower;
	// interpolate between full power and suppressed power
	a = mix(oldA * windPow, a * windPow, delayedWindPower * clamp(1 - facingWind, 0.0, 1.0));
	
	// compute orientations
	vec3 windTangent = vec3(-windDir.z, windDir.y, windDir.x); 
	vec4 rotation1 = quatAxisAngle(windTangent, a); // rotation away from wind
	vec4 rotation2 = quatAroundY(b); // rotation around parent

	// mix 'em (TODO is this sufficient or do we need SLERP?)
	return mix(rotation1, rotation2, 1.0 - abs(facingWind));  
} 

vec4 getOrientation(unsigned int idx)
{
	vec4 thisQuat = tree.branches[idx].orientation;

	unsigned int parentIdx = tree.branches[idx].parentIdx;
	if (branchIdx != parentIdx) // isn't trunk
	{
		//TODO bend branch according to wind
		vec4 accumulatedQuat = multQuat(thisQuat, getOrientation(parentIdx));

		return accumulatedQuat;
	}

	return thisQuat; // is trunk
}

void main(){
    passUVCoord = uvCoordAttribute;

    Branch thisBranch = tree.branches[ hierarchyAttribute.x ];

	// retrieve (wind manipulation simulated) orientation up until parent branch
	vec4 parentOrientation = getOrientation( thisBranch.parentIdx );

    vec3 pos = applyQuat(positionAttribute.xyz, parentOrientation);
	pos = applyQuat( thisBranch.orientation, pos.xyz );
	
	//vec4 pos = positionAttribute;
	vec4 worldPos = (model * vec4(pos,1.0));

    passWorldPosition = worldPos.xyz;
    passPosition = (view * worldPos).xyz;
    
    gl_Position =  projection * view * model * pos;

    passWorldNormal = normalize( ( transpose( inverse( model ) ) * normalAttribute).xyz );
	passNormal = normalize( ( transpose( inverse( view * model ) ) * normalAttribute ).xyz );

	VertexOut.texCoord = passUVCoord;	
	VertexOut.normal = passNormal;
	VertexOut.position = passPosition;
}
