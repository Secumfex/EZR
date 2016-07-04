#version 430

// arbitrary size, just to make sure enough memory is allocated
#define MAX_NUM_BRANCHES 35
#define EPSILON 0.0000001
#define PI 3.1415926535897932384626433832795

struct Branch
{
	vec4  orientation;
	vec3  origin;
	float phase;
	float pseudoInertiaFactor;
};

uniform Tree 
{
	float phase;
	Branch[MAX_NUM_BRANCHES] branches;
};

 //!< in-variables
layout(location = 0) in vec4 positionAttribute;
layout(location = 1) in vec2 uvCoordAttribute;
layout(location = 2) in vec4 normalAttribute;
layout(location = 3) in vec4 tangentAttribute;
layout(location = 4) in uvec3 hierarchyAttribute;//!< contains the indices of this and the parent branches
layout(location = 5) in mat4 instancedModel;//!< contains the indices of this and the parent branches

//!< uniforms
//uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float simTime; //!< used for noise function
//uniform vec4  worldWindDirection; // global wind direction, .a is equal to wind power
uniform sampler2D windField;
uniform vec4 windFieldArea; //!< x,y --> begin coords (XZ-plane) z,w --> end coords( XZ-plane )
uniform float windPower;

// Input parameters for simulation (for some reason array of vecs doesn't work)
uniform Simulation{
	vec3 vAngleShiftFront;
	vec3 vAngleShiftBack;
	vec3 vAngleShiftSide; 
	vec3 vAmplitudesFront;
	vec3 vAmplitudesBack;
	vec3 vAmplitudesSide;
	float fFrequencyFront;
	float fFrequencyBack;
	float fFrequencySide;
};

//!< out-variables
out vec3 passWorldPosition;
out vec3 passPosition;
out vec2 passUVCoord;
out vec3 passWorldNormal;
out vec3 passNormal;
out vec3 passTangent;

// for geometry shader
out VertexData {
	vec2 texCoord;
	vec3 position;
	vec3 normal;
	vec3 tangent;
} VertexGeom;

uniform sampler2D heightMap;
uniform vec4 heightMapRange; //!< x,y --> begin coords (XZ-plane) z,w --> end coords( XZ-plane )

#define HEIGHT_SCALE 50.0
#define HEIGHT_BIAS -2.5

vec2 worldToHeightMapUV(vec4 worldPos)
{
	vec2 heightMapUV;
	heightMapUV.x = (worldPos.x - heightMapRange.x) / (heightMapRange.z - heightMapRange.x );
	heightMapUV.y = (worldPos.z - heightMapRange.y) / (heightMapRange.w - heightMapRange.y);
	return heightMapUV; 
}

float mix3(float a, float b, float c, float t)
{
	return 
		mix(
			mix(a, b, clamp(t+1,0.0,1.0)),
			c,
			clamp(t,0.0,1.0));
}

vec4 quatAxisAngle(vec3 axis, float angle)
{ 
	float sinha = sin(angle * 0.5);
	float cosha = cos(angle * 0.5);

	return	vec4(
		axis * sinha,
		cosha );

}

vec4 quatAroundY(float angle)
{
	float sinha = sin(angle * 0.5);
	float cosha = cos(angle * 0.5);
	return vec4(0, sinha, 0, cosha);
}

//mat3 quatToMatrix( vec4 q)
//{
//	// add: 15 = 6 + 9
//	// mul: 16 = 12 + 4
//	// div:	 1
//
//	float dxy = q.x * q.y * 2.0f;
//	float dxz = q.x * q.z * 2.0f;
//	float dyz = q.y * q.z * 2.0f;
//	float dwx = q.w * q.x * 2.0f;
//	float dwy = q.w * q.y * 2.0f;
//	float dwz = q.w * q.z * 2.0f;
//	
//	float x2 = q.x * q.x;
//	float y2 = q.y * q.y;
//	float z2 = q.z * q.z;
//	float w2 = q.w * q.w;
//	
//	mat3 r = mat3(
//			w2+x2-y2-z2,	dxy+dwz,		dxz-dwy,
//			dxy-dwz,		w2-x2+y2-z2,	dyz+dwz,
//			dxz-dwy,		dyz-dwx,		w2-x2-y2+z2 );				
//	return r;
//}

vec3 applyQuat(vec4 q, vec3 v)
{
	vec3 QuatVector = vec3(q.x, q.y, q.z);
	vec3 uv = cross(QuatVector, v);
	vec3 uuv = cross(QuatVector, uv);

	return v + ((uv * q.w) + uuv) * 2.0;
}

//vec4 multQuat(vec4 q, vec4 p) // p first, then q
//{
//	vec4 result;
//	result.w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;
//	result.x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
//	result.y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
//	result.z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
//	return result;
//}


// vec4 rotation(vec3 orig, vec3 dest)
// {
// 	float cosTheta = dot(orig, dest);
// 	vec3 rotationAxis;

// 	if(cosTheta < (-1.0 + EPSILON))
// 	{
// 		rotationAxis = cross(vec3(0.0, 0.0, 1.0), orig);
// 		if(length(rotationAxis) < EPSILON) // bad luck, they were parallel, try again!
// 		{
// 			rotationAxis = cross(vec3(1.0, 0.0, 0.0), orig);
// 		}

// 		rotationAxis = normalize(rotationAxis);
// 		return quatAxisAngle(rotationAxis, PI * 180.0);
// 	}

// 	// Implementation from Stan Melax's Game Programming Gems 1 article
// 	rotationAxis = cross(orig, dest);

// 	float s = sqrt((1.0 + cosTheta) * 2.0);
// 	float invs = 1.0 / s;

// 	return vec4(
// 		s * 0.5, 
// 		rotationAxis.x * invs,
// 		rotationAxis.y * invs,
// 		rotationAxis.z * invs);
// }

vec4 bendBranch( vec3 pos,           // object space
                 vec3 branchOrigin,  // object space
                 vec3 trunkDirection,//object space 
				 float branchPhase,  // this branch's animation phase
				 float treePhase,    // this tree's animation phase
				 float pseudoInertiaFactor, // some offset or sth
				 vec3 windDirection, // object space
				 vec3 windTangent    // object space
		       )  
{
	vec3 branchPos = pos - branchOrigin;
	
	// determine branch orientation relative to the wind
	float dota = dot(-normalize(branchPos), windDirection);
	float dotb = dot(-normalize(branchPos), windTangent);

	// calculate parameters for simualation rules
	float t = dota * 0.5f + 0.5f;
	vec3 amplitudes  = mix(vAmplitudesBack, vAmplitudesFront, t);
	vec3 angleShift = mix(vAngleShiftBack, vAngleShiftFront, t);
	
	float amplitude0  = mix3(amplitudes.x, amplitudes.y, amplitudes.z, pseudoInertiaFactor);
	float angleShift0 = mix3(angleShift.x, angleShift.y, angleShift.z, pseudoInertiaFactor);

	float frequency0 = 1.0;
	if (dota > 0)
	{
		frequency0 = fFrequencyFront;
	}
	else
	{
		 frequency0 = fFrequencyBack;
	}
	float amplitude1 = vAmplitudesSide.y;
	float angleShift1 = vAngleShiftSide.y * dotb;
	float frequency1 = fFrequencySide;

	// cacluate quaternion representing bending of the branch due to wind load
	// along direction of the wind
	vec4 q0 = quatAxisAngle(windTangent,    angleShift0 + amplitude0 * sin((branchPhase + treePhase + simTime) * frequency0));
	
	// cacluate quaternion representing bending of the branch perpendicular to main trunk
	vec4 q1 = quatAxisAngle(trunkDirection, angleShift1 + amplitude1 * sin((branchPhase + treePhase + simTime) * frequency1));
	
	// combine bending
	return normalize(mix(q1, q0, abs(dota))); 
} 

int countNonZero(uvec3 hierarchy)
{
	int result = 0;
	result += int( (hierarchy.x != 0) );
	result += int( (hierarchy.y != 0) );
	result += int( (hierarchy.z != 0) );
	//result += int( (hierarchy.w != 0) );
	return result;
}


void main(){
	int numParents  = countNonZero(hierarchyAttribute); // amount of branch indices that are not root

	// initial properties of branch
	float tree_phase = phase;

	vec3 branch_origin   = branches[hierarchyAttribute[0]].origin;         // the vertex's branch origin
	vec4 branch_orientation = branches[hierarchyAttribute[0]].orientation; // the vertex's branch orientation
	vec3 vertex_pos = branch_origin + applyQuat(branch_orientation, positionAttribute.xyz); // initial vertex position
	vec3 vertex_normal = applyQuat(branch_orientation, normalAttribute.xyz);

	vec2 windFieldSampleCoords =  ((instancedModel[3]).xz - windFieldArea.xy) / (windFieldArea.zw - windFieldArea.xy);
	vec4 worldWindDirection = texture(windField, windFieldSampleCoords );
	worldWindDirection.xyz = worldWindDirection.xzy;
	worldWindDirection.a = length(worldWindDirection.xyz);
	worldWindDirection.xyz = normalize(worldWindDirection.xyz);

	vec3 wind_direction_model = (inverse(instancedModel) * vec4(worldWindDirection.xyz,0.0)).xyz; // wind direction in object space
	vec3 wind_tangent_model = vec3(-wind_direction_model.z, wind_direction_model.y, wind_direction_model.x); 


	for (int i = 0; i <= numParents; i++) //not trunk
	{	
		//simulated properties of parent
		vec4 branch_orientation_offset = vec4(0,0,0,1);
		vec4 branch_orientation = vec4(0,0,0,1);
		vec3 branch_origin = vec3(0,0,0);
		float branch_phase = 0.0;
		float branch_pseudoInertiaFactor = 1.0;
		float branch_weight = 1.0;

		if ( i == 0)
		{
			branch_weight = clamp(distance(branch_origin, vertex_pos),0.0,1.0);
		}

		if ( hierarchyAttribute[i] > 0 )
		{
			branch_origin      = branches[hierarchyAttribute[i]].origin;
			branch_phase       = branches[hierarchyAttribute[i]].phase;
			branch_pseudoInertiaFactor = branches[hierarchyAttribute[i]].pseudoInertiaFactor;

			branch_orientation_offset = bendBranch(
				vertex_pos,
				branch_origin,
				vec3(0,1,0),
				branch_phase,
				tree_phase,
				branch_pseudoInertiaFactor
				, wind_direction_model
				, wind_tangent_model
			);
		}
		vec3 new_pos =applyQuat(branch_orientation_offset, vertex_pos - branch_origin) + branch_origin;
		vec3 new_normal = applyQuat(branch_orientation_offset, vertex_normal);
		vertex_normal = mix(vertex_normal, new_normal, branch_weight);
		vertex_pos = mix(vertex_pos, new_pos, branch_weight);
	}

	//float world_wind_power = sin(simTime) * (worldWindDirection.a / 2.0f) + worldWindDirection.a / 2.0f + (0.25f * sin(2.0f * worldWindDirection.a * simTime + 0.25f)); 
	float world_wind_power = worldWindDirection.a * windPower;
	vec4 trunk_rotation_quat = vec4(0,0,0,1);
	if ( world_wind_power > 0.0001)
	{
		trunk_rotation_quat = quatAxisAngle(wind_tangent_model, world_wind_power);
	}

	vec4 trunk_rotated_vertex_pos = vec4( applyQuat(trunk_rotation_quat, vertex_pos),1.0);
	float windRotationWeight = clamp( length(vertex_pos)/4.0, 0.0, 1.0);
	
	vec4 final_vertex_pos = mix(vec4(vertex_pos,1.0), trunk_rotated_vertex_pos, windRotationWeight);

	vec4 worldPos = instancedModel * final_vertex_pos;

		// adjust y coord
	worldPos.y += texture(heightMap, worldToHeightMapUV(worldPos)).x * HEIGHT_SCALE + HEIGHT_BIAS;


    passWorldPosition = worldPos.xyz;
    passPosition = (view * worldPos).xyz;
    gl_Position =  projection * view * worldPos;
    passUVCoord = uvCoordAttribute;

    passWorldNormal = normalize( ( transpose( inverse( instancedModel ) ) * normalAttribute).xyz );
	
	mat4 normalMatrix = transpose( inverse( view * instancedModel ) ) ;
	passNormal = normalize( normalMatrix * vec4(vertex_normal,0.0) ).xyz;
	passTangent = normalize( normalMatrix * vec4(tangentAttribute.xyz,0.0) ).xyz;

    VertexGeom.texCoord = passUVCoord;
	VertexGeom.position = passPosition;
	VertexGeom.normal   = passNormal;
	VertexGeom.tangent  =  passTangent;
}