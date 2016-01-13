#ifndef TREE_H
#define TREE_H

#include <glm/glm.hpp>
#include <vector>

namespace TreeAnimation
{
// some elastic modulus constants
static const float E_RED_OAK = 12.5f; // GPa
static const float E_SOFTWOOD_TIMBER = 7.0f; // GPa
static const float E_BEECH = 14.3f; // GPa
static const float E_APPLE = 8.76; // GPa

class Tree
{
public:
	struct Branch
	{
		glm::vec3 origin; //!< in branch-space of in object-space
		glm::vec3 direction; //!< direction of the branch in object-space

		float base_width;
		float thickness; //!< base_width - top_width (implicitly)
		float length;
		float stiffness; //!< computed by thickness, length and the tree-specific elastic-modulus-constant E
		
		Branch* parent; //!< nullptr if trunk
		
		std::vector<Branch* > children;
		unsigned int idx; //!< an unique index identifying this branch
	} m_trunk; 

	// static functions
	static float computeStiffness(float b, float t, float l, float E); //!< base_width, thickness, length, E
	static Tree* generateTree(float approxHeight, float approxWidth, int numMainBranches, int numSubBranches, std::vector<Tree::Branch*>* branchPtrs = nullptr);

	// public members 
	float m_E; //!< elastic modulus of this tree species

	// methods
	Tree(float thickness, float length, float stiffness);
	~Tree();

	Branch* addBranch(Branch* parent, glm::vec3 direction, float posOnParent, float thickness, float length, float stiffness);
	Branch* addRandomBranch(TreeAnimation::Tree::Branch* parent, float rPosMin, float rPosMax, float rLengthMax, float rLengthMin, float rPitchMin, float rPitchMax);
	
protected:
	unsigned int m_nextBranchIdx;
};

} // TreeAnimation
#endif