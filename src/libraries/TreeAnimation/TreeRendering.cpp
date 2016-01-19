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

Renderable* TreeAnimation::generateFoliage( TreeAnimation::Tree::Branch* branch, int numLeafs, const aiScene* foliageModel)
	{		
		Renderable* renderable = new Renderable();
		std::vector<float> positions;
		std::vector<unsigned int> indices;
		std::vector<float> normals;
		std::vector<float> uvs;

		//TODO the foliagemodel-variant of this stuff

		for ( int i = 0; i < numLeafs; i++)
		{
			int nextIdx = positions.size()/3;
			glm::vec3 v[4];
			auto addVert = [&](float x, float y, float z, int temp) 
			{
				positions.push_back(x);
				positions.push_back(y);
				positions.push_back(z);
				v[temp] = glm::vec3(x,y,z);
			};

			auto addUV = [&](float s, float t) 
			{
				uvs.push_back(s);
				uvs.push_back(t);
			};

			float rWidth = ((float) rand()) / ((float) RAND_MAX)* 0.2 + 0.03; //0.03..0.23
			float rHeight = ((float) rand()) / ((float) RAND_MAX)* 0.2 + 0.03; //0.03..0.23
			
			float rOffsetX = ((float) rand()) / ((float) RAND_MAX) * (branch->length / 2.0) - (branch->length / 4.0); //0..0.1
			float rOffsetY = ((float) rand()) / ((float) RAND_MAX) * branch->length; //-branchLength/4 .. branchLegnth/4
			float rOffsetZ = ((float) rand()) / ((float) RAND_MAX) * branch->thickness * 2.0f - branch->thickness;

			addVert(-rWidth + rOffsetX,-rHeight+ rOffsetY,rOffsetZ,0);
			addVert(-rWidth+ rOffsetX,rHeight + rOffsetY,rOffsetZ,1);
			addVert(rWidth+ rOffsetX,rHeight+ rOffsetY,rOffsetZ,2);
			addVert(rWidth+ rOffsetX,-rHeight+ rOffsetY,rOffsetZ,3);
			
			addUV(0.0f, 0.0f);
			addUV(0.0f, 1.0f);
			addUV(1.0f, 1.0f);
			addUV(1.0f, 0.0f);

			glm::vec3 n[4];
			auto addNorm = [&](int i0, int i1, int i2, int temp) 
			{
				n[temp] = glm::normalize(glm::cross(v[i1]- v[i0], v[i2]-v[i0]));
				if ( abs(n[temp].x) < 0.0000001){ n[temp].x = 0;}
				normals.push_back(n[temp].x);
				normals.push_back(n[temp].y);
				normals.push_back(n[temp].z);
			};
			addNorm(0,3,1,0);
			addNorm(1,0,2,1);
			addNorm(2,1,3,2);
			addNorm(3,2,0,3);

			indices.push_back(nextIdx);
			indices.push_back(nextIdx+1);
			indices.push_back(nextIdx+2);

			indices.push_back(nextIdx+2);
			indices.push_back(nextIdx+3);
			indices.push_back(nextIdx);
		}

	    glGenVertexArrays(1, &renderable->m_vao);
		glBindVertexArray(renderable->m_vao);

		renderable->m_positions.m_vboHandle = Renderable::createVbo(positions, 3, 0);
		renderable->m_positions.m_size = positions.size() / 3;

		renderable->m_uvs.m_vboHandle = Renderable::createVbo(uvs, 2, 1);
		renderable->m_uvs.m_size = normals.size() / 2;

		renderable->m_normals.m_vboHandle = Renderable::createVbo(normals, 3, 2);
		renderable->m_normals.m_size = normals.size() / 3;

		renderable->m_indices.m_vboHandle = Renderable::createIndexVbo(indices);
		renderable->m_indices.m_size = indices.size();
				
		renderable->setDrawMode(GL_TRIANGLES);

		std::vector<unsigned int> hierarchy;
		for ( int i = 0; i < renderable->m_positions.m_size; i++)
		{
			TreeAnimation::Tree::hierarchy(branch, &hierarchy); // add hierarchy once for every vertex
		}

		// add another vertex attribute which contains the tree hierarchy
		Renderable::createVbo<unsigned int>(hierarchy, 3, 4, GL_UNSIGNED_INT, true); 

		glBindVertexArray(0); 

		return renderable;
	};