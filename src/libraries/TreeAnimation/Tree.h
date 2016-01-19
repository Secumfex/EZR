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
		float phase; //!< random phase shift used for simulation
		
		Branch* parent; //!< nullptr if trunk
		
		std::vector<Branch* > children;
		unsigned int idx; //!< an unique index identifying this branch
	} m_trunk; 

	// static functions
	static float computeStiffness(float b, float t, float l, float E); //!< base_width, thickness, length, E
	static Tree* generateTree(float approxHeight, float approxWidth, int numMainBranches, int numSubBranches, std::vector<Tree::Branch*>* branchPtrs = nullptr);
	static glm::uvec3 hierarchy(TreeAnimation::Tree::Branch* branch, std::vector<unsigned int>* hierarchy = nullptr);
	static std::vector<Branch*> getSubBranchVector(Tree::Branch* branch); //!< get a vector of this pointers to this branch and all branches below in the hierarchy

	// public members 
	float m_E; //!< elastic modulus of this tree species
	float m_phase; //!< random phase shift used for simulation
	std::vector<Branch*> m_branchesIndexed; //!< contains all branches, with the vector's index corresponding to the branch's unique idx

	// methods
	Tree(float length, float base_width, float thickness, float ElasticityConstant, float phase = 0.0f);
	~Tree();

	Branch* addBranch(TreeAnimation::Tree::Branch* parent, glm::vec3 direction, float posOnParent, float length, float base_width, float relThickness = 1.0f, float phase = 0.0f);
	Branch* addRandomBranch(TreeAnimation::Tree::Branch* parent, float rPosMin, float rPosMax, float rLengthMin, float rLengthMax, float rPitchMin, float rPitchMax);

protected:

public:
	static float s_r_pos_min_main, s_r_pos_max_main, s_r_pos_min_sub, s_r_pos_max_sub;
	static float s_r_length_min_sub, s_r_length_max_sub, s_r_length_min_main, s_r_length_max_main;
	static float s_r_pitch_min_main, s_r_pitch_max_main, s_r_pitch_min_sub, s_r_pitch_max_sub;
};

} // TreeAnimation
#endif