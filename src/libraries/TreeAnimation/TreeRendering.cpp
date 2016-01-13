#include "TreeRendering.h"

#include <stdlib.h>
#include <functional>

#include <assimp/scene.h>
#include <Importing/AssimpTools.h>
#include <glm/gtx/transform.hpp>

Renderable* TreeAnimation::generateRenderable(TreeAnimation::Tree::Branch* branch, const aiScene* branchModel)
{
	Renderable* renderable = nullptr;

	if (branchModel != NULL)
	{
		glm::mat4 transform = glm::scale( glm::vec3(branch->thickness / 2.0f, branch->length, branch->thickness / 2.0f) );
		auto renderables = AssimpTools::createSimpleRenderablesFromScene(branchModel, transform);
		renderable = renderables.at(0).renderable;
	}else{
		// generate regular Truncated Cone
		renderable = new TruncatedCone( branch->length, branch->thickness / 2.0f, 0.0f, 20, 0.0f);
	}

	renderable->bind();

	std::vector<unsigned int> hierarchy;

	for ( int i = 0; i < renderable->m_positions.m_size; i++)
	{
		Tree::hierarchy(branch, &hierarchy); // add hierarchy once for every vertex
	}

	// add another vertex attribute which contains the tree hierarchy
	Renderable::createVbo<unsigned int>(hierarchy, 3, 4, GL_UNSIGNED_INT, true); 
	
	glBindVertexArray(0); 

	return renderable;
}
