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

void TreeAnimation::generateFoliageVertexData( TreeAnimation::Tree::Branch* branch, int numLeafs, TreeAnimation::FoliageVertexData& target)
{		
	for ( int i = 0; i < numLeafs; i++)
	{
		int nextIdx = target.positions.size()/3;
		glm::vec3 v[4];
		auto addVert = [&](float x, float y, float z, int temp) 
		{
			target.positions.push_back(x);
			target.positions.push_back(y);
			target.positions.push_back(z);
			v[temp] = glm::vec3(x,y,z);
		};

		auto addUV = [&](float s, float t) 
		{
			target.uvs.push_back(s);
			target.uvs.push_back(t);
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
			target.normals.push_back(n[temp].x);
			target.normals.push_back(n[temp].y);
			target.normals.push_back(n[temp].z);
		};
		addNorm(0,3,1,0);
		addNorm(1,0,2,1);
		addNorm(2,1,3,2);
		addNorm(3,2,0,3);

		// add hierarchy once for every vertex
		TreeAnimation::Tree::hierarchy(branch, &target.hierarchy);
		TreeAnimation::Tree::hierarchy(branch, &target.hierarchy);
		TreeAnimation::Tree::hierarchy(branch, &target.hierarchy);
		TreeAnimation::Tree::hierarchy(branch, &target.hierarchy);

		target.indices.push_back(nextIdx);
		target.indices.push_back(nextIdx+1);
		target.indices.push_back(nextIdx+2);

		target.indices.push_back(nextIdx+2);
		target.indices.push_back(nextIdx+3);
		target.indices.push_back(nextIdx);

	}
}

Renderable* TreeAnimation::generateFoliageRenderable(TreeAnimation::FoliageVertexData& source)
{
	Renderable* renderable = new Renderable();

	glGenVertexArrays(1, &renderable->m_vao);
	glBindVertexArray(renderable->m_vao);

	renderable->m_positions.m_vboHandle = Renderable::createVbo(source.positions, 3, 0);
	renderable->m_positions.m_size = source.positions.size() / 3;

	renderable->m_uvs.m_vboHandle = Renderable::createVbo(source.uvs, 2, 1);
	renderable->m_uvs.m_size = source.normals.size() / 2;

	renderable->m_normals.m_vboHandle = Renderable::createVbo(source.normals, 3, 2);
	renderable->m_normals.m_size = source.normals.size() / 3;

	renderable->m_indices.m_vboHandle = Renderable::createIndexVbo(source.indices);
	renderable->m_indices.m_size = source.indices.size();
				
	renderable->setDrawMode(GL_TRIANGLES);

	// add another vertex attribute which contains the tree hierarchy
	Renderable::createVbo<unsigned int>(source.hierarchy, 3, 4, GL_UNSIGNED_INT, true); 

	glBindVertexArray(0); 

	return renderable;
}

#include <Core/DebugLog.h>
void TreeAnimation::generateBranchVertexData(TreeAnimation::Tree::Branch* branch, TreeAnimation::BranchesVertexData& target, const aiScene* scene)
{
	if (scene != NULL)
	{
		glm::mat4 transform = glm::scale( glm::vec3(branch->thickness / 2.0f, branch->length, branch->thickness / 2.0f) );
		
		int indexOffset = target.positions.size() / 3;

		auto vertexData = AssimpTools::createVertexDataInstancesFromScene(scene, transform)[0];
		target.positions.insert(target.positions.end(), vertexData.positions.begin(), vertexData.positions.end());
		target.uvs.insert(target.uvs.end(), vertexData.uvs.begin(), vertexData.uvs.end());
		target.normals.insert(target.normals.end(), vertexData.normals.begin(), vertexData.normals.end());
		target.tangents.insert(target.tangents.end(), vertexData.tangents.begin(), vertexData.tangents.end());

		for ( int i = 0; i < vertexData.indices.size(); i++)
		{
			vertexData.indices[i] += indexOffset;
		}
		target.indices.insert(target.indices.end(), vertexData.indices.begin(), vertexData.indices.end());

		for (int v = 0; v < vertexData.positions.size(); v = v + 3)
		{
			TreeAnimation::Tree::hierarchy(branch, &target.hierarchy);
		}
	}else{
		// generate regular Truncated Cone
		auto vertexData = TruncatedCone::generateVertexData( branch->length, branch->thickness / 2.0f, 0.0f, 20, 0.0f);

		int indexOffset = target.positions.size() / 3;

		target.positions.insert(target.positions.end(), vertexData.positions.begin(), vertexData.positions.end());
		target.uvs.insert(target.uvs.end(), vertexData.uv_coords.begin(), vertexData.uv_coords.end());
		target.normals.insert(target.normals.end(), vertexData.normals.begin(), vertexData.normals.end());
		//target.tangents.insert(target.tangents.end(), vertexData.tangents.begin(), vertexData.tangents.end());

		for ( int i = 0; i < vertexData.indices.size(); i++)
		{
			vertexData.indices[i] += indexOffset;
		}
		target.indices.insert(target.indices.end(), vertexData.indices.begin(), vertexData.indices.end());

		for (int v = 0; v < vertexData.positions.size(); v = v + 3)
		{
			TreeAnimation::Tree::hierarchy(branch, &target.hierarchy);
		}
	}
}

Renderable* TreeAnimation::generateBranchesRenderable(TreeAnimation::BranchesVertexData& source)
{
	Renderable* renderable = new Renderable();

	std::vector<AssimpTools::VertexData> vd(1);
	vd[0].indices = source.indices;
	vd[0].positions= source.positions;
	vd[0].uvs = source.uvs;
	vd[0].normals = source.normals;
	vd[0].tangents = source.tangents;
	renderable = AssimpTools::createSimpleRenderablesFromVertexDataInstances(vd)[0];

	renderable->bind();
	// add another vertex attribute which contains the tree hierarchy
	Renderable::createVbo<unsigned int>(source.hierarchy, 3, 4, GL_UNSIGNED_INT, true); 

	renderable->unbind(); 

	return renderable;
}