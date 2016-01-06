#ifndef TREE_H
#define TREE_H

#include <glm/glm.hpp>
#include <vector>

namespace TreeAnimation
{


class Tree
{
public:
	static float computeStiffness(float b, float t, float l, float E);

	struct Branch
	{
		glm::vec3 origin;
		glm::vec3 direction; //!< direction of the branch 
		float thickness;
		float length;
		float stiffness; //!< computed by thickness, length and the tree-specific elastic-modulus-constant E
		Branch* parent;
		std::vector<Branch* > children;
	} m_trunk; 

	float m_E; //!< elastic modulus of this tree species

	Tree(float thickness, float length, float stiffness);
	~Tree();

	Branch* addBranch(Branch* parent, glm::vec3 direction, float posOnParent, float thickness, float length, float stiffness);
};

} // TreeAnimation
#endif