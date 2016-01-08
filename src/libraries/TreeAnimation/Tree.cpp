#include "Tree.h"

#include <stdlib.h>
#include <functional>

namespace TreeAnimation
{

	float Tree::computeStiffness(float b, float t, float l, float E)
	{
		return (E * b * pow(t, 3.0f)) / (4.0f * pow(l, 3.0f));
	}

	Tree::Tree(float thickness, float length, float stiffness)
	{
		m_trunk.idx = 0;
		m_nextBranchIdx = 1;
		m_trunk.origin = glm::vec3(0.0f, 0.0f, 0.0f);
		m_trunk.direction = glm::vec3(0.0f, 1.0f, 0.0f); //!< direction of the branch 
		m_trunk.thickness = thickness;
		m_trunk.length = length;
		m_trunk.stiffness = stiffness; //!< computed by thickness, length and the tree-specific elastic-modulus-constant E
		m_trunk.parent = nullptr; 
	}

	Tree::~Tree()
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


	Tree::Branch* Tree::addBranch(Tree::Branch* parent, glm::vec3 direction, float posOnParent, float thickness, float length, float stiffness)
	{
		Tree::Branch* branch = new Tree::Branch;
		branch->length = length;
		branch->origin = (posOnParent * parent->length) * glm::vec3(0.0f,1.0f,0.0f); // branch space of parent
		//branch->origin = parent->origin + (posOnParent * parent->length) * parent->direction; // object space of tree
		branch->direction = direction;
		branch->thickness = thickness;
		branch->stiffness = stiffness;
		branch->parent = parent;

		branch->idx = m_nextBranchIdx;
		m_nextBranchIdx++;

		parent->children.push_back(branch);

		return branch;
	}
}