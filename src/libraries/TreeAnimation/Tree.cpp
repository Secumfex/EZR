#include "Tree.h"

#include <stdlib.h>
#include <functional>

float TreeAnimation::Tree::computeStiffness(float b, float t, float l, float E)
{
	return (E * b * pow(t, 3.0f)) / (4.0f * pow(l, 3.0f));
}

TreeAnimation::Tree::Tree(float thickness, float length, float stiffness)
{
	m_trunk.idx = 0;
	m_nextBranchIdx = 1;
	m_trunk.origin = glm::vec3(0.0f, 0.0f, 0.0f);
	m_trunk.direction = glm::vec3(0.0f, 1.0f, 0.0f); //!< direction of the branch 
	m_trunk.thickness = thickness;
	m_trunk.length = length;
	m_trunk.stiffness = stiffness; //!< computed by thickness, length and the tree-specific elastic-modulus-constant E
	m_trunk.parent = nullptr;

	m_E = E_RED_OAK;
}

TreeAnimation::Tree::~Tree()
{
	std::function<void(Branch*)> deleteBranchesRecursively = [&](Branch* branch)
	{
		if ( !branch->children.empty() )
		{
			for ( auto b : branch->children )
			{
				deleteBranchesRecursively(b);
			}
		}
			
		if ( branch->parent != nullptr) // not trunk
		delete branch;
	};

	deleteBranchesRecursively(&m_trunk);
}


TreeAnimation::Tree::Branch* TreeAnimation::Tree::addBranch(TreeAnimation::Tree::Branch* parent, glm::vec3 direction, float posOnParent, float thickness, float length, float stiffness)
{
	Tree::Branch* branch = new Tree::Branch;
	branch->length = length;
	//branch->origin = (posOnParent * parent->length) * glm::vec3(0.0f,1.0f,0.0f); // branch space of parent
	branch->origin = parent->origin + (posOnParent * parent->length) * parent->direction; // object space of tree
	branch->direction = direction;
	branch->thickness = thickness;
	branch->stiffness = stiffness;
	branch->parent = parent;

	branch->idx = m_nextBranchIdx;
	m_nextBranchIdx++;

	parent->children.push_back(branch);

	return branch;
}

#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

TreeAnimation::Tree::Branch* TreeAnimation::Tree::addRandomBranch(TreeAnimation::Tree::Branch* parent, float rPosMin, float rPosMax, float rLengthMin, float rLengthMax,float rPitchMin, float rPitchMax)
{
	// randomization values
	float r1 = ((float) rand()) / ((float) RAND_MAX);
	float r2 = ((float) rand()) / ((float) RAND_MAX);
	float rLength = ( r1 * (rLengthMax - rLengthMin)) + rLengthMin;
	float rPitchAngle = ( r2 * (rPitchMax - rPitchMin) ) + rPitchMin; // should be between 0° and 180°
	float rYawAngle = ((float) rand()) / ((float) RAND_MAX) * 2.f * glm::pi<float>() - glm::pi<float>(); //between -180° and 180°
	// float rYawAngle = 0.0f;
	float rPos = (((float) rand()) / ((float) RAND_MAX) * (rPosMax - rPosMin)) + rPosMin ;

	// optimal: 90° to parent, assuming parent is pointing in (0,1,0) in object space
	glm::vec3 optimalDirection = glm::vec3(1.0f,0.0f,0.0f);
	glm::vec3 rDirection = glm::normalize( glm::rotateY( glm::rotateZ( optimalDirection, rPitchAngle), rYawAngle)); // apply randomization
		
	// retrieve object-space orientation of parent
	glm::vec3 parentDirection= parent->direction;
	glm::quat parentRotation = glm::rotation(glm::vec3(0.0f,1.0f,0.0f), parent->direction);
	float thickness = (1.0f - rPos) * parent->thickness * 0.5f; 
		
	// apply parent orientation to this branch direction
	rDirection = (glm::rotate(parentRotation, rDirection));

	// add branch
	return addBranch(
		parent,
		rDirection,
		rPos,
		thickness,
		rLength,
		TreeAnimation::Tree::computeStiffness( 
			(1.0f - rPos) * parent->thickness,
			(1.0f - rPos) * parent->thickness,
			rLength,
			m_E)
		); ;
};

float TreeAnimation::Tree::s_r_pos_min_main		= 0.5f;
float TreeAnimation::Tree::s_r_pos_max_main		= 0.75f;
float TreeAnimation::Tree::s_r_pos_min_sub		= 0.5f;
float TreeAnimation::Tree::s_r_pos_max_sub		= 1.0f;
float TreeAnimation::Tree::s_r_length_min_main	= 0.25f;
float TreeAnimation::Tree::s_r_length_max_main	= 0.5f;
float TreeAnimation::Tree::s_r_length_min_sub	= 0.25f;
float TreeAnimation::Tree::s_r_length_max_sub	= 0.5f;
float TreeAnimation::Tree::s_r_pitch_min_main	= 40.0f;
float TreeAnimation::Tree::s_r_pitch_max_main	= 50.0f;
float TreeAnimation::Tree::s_r_pitch_min_sub	= 40.0f;
float TreeAnimation::Tree::s_r_pitch_max_sub	= 50.0f;

TreeAnimation::Tree* TreeAnimation::Tree::generateTree(float approxHeight, float approxWidth, int numMainBranches, int numSubBranches, std::vector<Tree::Branch*>* branchPtrs)
{
	float rWidth = approxWidth + ((float) rand() / (float) RAND_MAX) * (0.1f * approxWidth) - (0.05f * approxWidth); //random offset of 10%
	float rThickness = rWidth - abs(approxWidth - rWidth);

	float rHeight = approxHeight + ((float) rand() / (float) RAND_MAX) * (0.1f * approxHeight) - (0.05f * approxHeight); //random offset of 10%
	TreeAnimation::Tree* tree = new Tree(rWidth, rHeight, Tree::computeStiffness(rWidth, rThickness, rHeight, E_RED_OAK) );
	
	if ( branchPtrs != nullptr){ branchPtrs->push_back(&tree->m_trunk); }
	
	for ( int i = 0; i < numMainBranches; i++)
	{
		auto branch = tree->addRandomBranch(
			&tree->m_trunk,
			s_r_pos_min_main,
			s_r_pos_max_main,
			tree->m_trunk.length * s_r_length_min_main,
			tree->m_trunk.length * s_r_length_max_main,
			glm::radians(s_r_pitch_min_main),
			glm::radians(s_r_pitch_max_main));
		if ( branchPtrs != nullptr){ branchPtrs->push_back(branch); }
		
		for ( int j = 0; j < numSubBranches; j++)
		{
			auto subBranch = tree->addRandomBranch( 
				branch,
				s_r_pos_min_sub,
				s_r_pos_min_sub,
				branch->length * s_r_length_min_sub,
				branch->length * s_r_length_max_sub,
				glm::radians(s_r_pitch_min_sub),
				glm::radians(s_r_pitch_max_sub));
			if ( branchPtrs != nullptr){ branchPtrs->push_back(subBranch); }
		}
	}

	return tree;
} 

