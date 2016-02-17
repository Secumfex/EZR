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

		float rWidth = ((float) rand()) / ((float) RAND_MAX)* 0.2f + 0.03f; //0.03..0.23
		float rHeight = ((float) rand()) / ((float) RAND_MAX)* 0.2f + 0.03f; //0.03..0.23
			
		float rOffsetX = ((float) rand()) / ((float) RAND_MAX) * (branch->length / 2.0f) - (branch->length / 4.0f); //0..0.1
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
			if ( abs(n[temp].x) < 0.0000001f){ n[temp].x = 0.0f;}
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

void TreeAnimation::generateFoliageGeometryShaderVertexData( TreeAnimation::Tree::Branch* branch, int numLeafs, TreeAnimation::FoliageVertexData& target)
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
	
		float rOffsetX = ((float) rand()) / ((float) RAND_MAX) * (branch->length / 2.0f) - (branch->length / 4.0f); //-branchLength/4 .. branchLegnth/4
		float rOffsetY = ((float) rand()) / ((float) RAND_MAX) * branch->length; 
		float rOffsetZ = ((float) rand()) / ((float) RAND_MAX) * (branch->length / 2.0f) - (branch->length / 4.0f);

		addVert(rOffsetX, rOffsetY, rOffsetZ, 0);

		float rWidth = ((float) rand()) / ((float) RAND_MAX)* 0.2f + 0.03f; //0.03..0.23
		float rHeight = ((float) rand()) / ((float) RAND_MAX)* 0.2f + 0.03f; //0.03..0.23

		addUV(rWidth, rHeight);

		glm::vec3 n[4];

		auto addNorm = [&](float vX, float vY, float vZ, int temp) 
		{
			glm::vec3 centerToPoint = glm::vec3(vX, vY, vZ) - glm::vec3(0.0f, branch->length / 2.0f, 0.0f);
			n[temp] = glm::normalize(centerToPoint);
			target.normals.push_back(n[temp].x);
			target.normals.push_back(n[temp].y);
			target.normals.push_back(n[temp].z);
		};

		addNorm(v[0].x, v[0].y, v[0].z, 0);

		// add hierarchy once for every vertex
		TreeAnimation::Tree::hierarchy(branch, &target.hierarchy);

		target.indices.push_back(nextIdx);
	}
}

Renderable* TreeAnimation::generateFoliageGeometryShaderRenderable(TreeAnimation::FoliageVertexData& source)
{
	auto r = generateFoliageRenderable(source);
	r->setDrawMode(GL_POINTS);
	return r;
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

		for (unsigned int i = 0; i < vertexData.indices.size(); i++)
		{
			vertexData.indices[i] += indexOffset;
		}
		target.indices.insert(target.indices.end(), vertexData.indices.begin(), vertexData.indices.end());

		for (unsigned int v = 0; v < vertexData.positions.size(); v = v + 3)
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

		for (unsigned int i = 0; i < vertexData.indices.size(); i++)
		{
			vertexData.indices[i] += indexOffset;
		}
		target.indices.insert(target.indices.end(), vertexData.indices.begin(), vertexData.indices.end());

		for (unsigned int v = 0; v < vertexData.positions.size(); v = v + 3)
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
#include <glm/gtx/quaternion.hpp>
void TreeAnimation::updateTreeUniforms(ShaderProgram& shaderProgram, TreeAnimation::Tree* tree)
{	
	shaderProgram.update("tree.phase", tree->m_phase);

	// upload tree uniforms
	for (unsigned int i = 0; i < tree->m_branchesIndexed.size(); i++)
	{
		std::string prefix = "tree.branches[" + DebugLog::to_string(i) + "].";

		shaderProgram.update(prefix + "origin", tree->m_branchesIndexed[i]->origin);
		shaderProgram.update(prefix + "phase", tree->m_branchesIndexed[i]->phase);	
		shaderProgram.update(prefix + "pseudoInertiaFactor", 1.0f);
			
		// orientation is computed from object space direction relative to optimal branch axis
		glm::quat orientation = glm::rotation(glm::vec3(0.0f,1.0f,0.0f), tree->m_branchesIndexed[i]->direction);
		glm::vec4 quatAsVec4 = glm::vec4(orientation.x, orientation.y, orientation.z, orientation.w);
		shaderProgram.update(prefix + "orientation", quatAsVec4);
	}
}

void TreeAnimation::updateSimulationUniforms(ShaderProgram& shaderProgram, TreeAnimation::SimulationProperties& simulation)
{
	shaderProgram.update("vAngleShiftFront", simulation.angleshifts[0]); //front
	shaderProgram.update("vAngleShiftBack", simulation.angleshifts[1]); //back
	shaderProgram.update("vAngleShiftSide", simulation.angleshifts[2]); //side

	shaderProgram.update("vAmplitudesFront", simulation.amplitudes[0]); //front
	shaderProgram.update("vAmplitudesBack", simulation.amplitudes[1]); //back
	shaderProgram.update("vAmplitudesSide", simulation.amplitudes[2]); //side

	shaderProgram.update("fFrequencyFront", simulation.frequencies.x); //front
	shaderProgram.update("fFrequencyBack", simulation.frequencies.y); //back
	shaderProgram.update("fFrequencySide", simulation.frequencies.z); //side
}

#include <glm/gtc/type_ptr.hpp>
void TreeAnimation::updateSimulationUniformsInBufferData(TreeAnimation::SimulationProperties& simulation, ShaderProgram::UniformBlockInfo& info, std::vector<float>& data)
{
	ShaderProgram::updateValuesInBufferData("vAngleShiftFront", glm::value_ptr(simulation.angleshifts[0]), 3, info, data);
	ShaderProgram::updateValuesInBufferData("vAngleShiftBack", glm::value_ptr(simulation.angleshifts[1]), 3, info, data);
	ShaderProgram::updateValuesInBufferData("vAngleShiftSide", glm::value_ptr(simulation.angleshifts[2]), 3, info, data);
	
	ShaderProgram::updateValuesInBufferData("vAmplitudesFront", glm::value_ptr(simulation.amplitudes[0]), 3, info, data);
	ShaderProgram::updateValuesInBufferData("vAmplitudesBack", glm::value_ptr(simulation.amplitudes[1]), 3, info, data);
	ShaderProgram::updateValuesInBufferData("vAmplitudesSide", glm::value_ptr(simulation.amplitudes[2]), 3, info, data);
	
	ShaderProgram::updateValuesInBufferData("fFrequencyFront", &simulation.frequencies.x, 1, info, data);
	ShaderProgram::updateValuesInBufferData("fFrequencyBack", &simulation.frequencies.y, 1, info, data);
	ShaderProgram::updateValuesInBufferData("fFrequencySide", &simulation.frequencies.z, 1, info, data);
}

void TreeAnimation::updateTreeUniformsInBufferData(TreeAnimation::Tree* tree, ShaderProgram::UniformBlockInfo& info, std::vector<float>& data)
{
	ShaderProgram::updateValuesInBufferData("phase", &tree->m_phase, 1, info, data);

	// upload tree uniforms
	for (unsigned int i = 0; i < tree->m_branchesIndexed.size(); i++)
	{
		std::string prefix = "branches[" + DebugLog::to_string(i) + "].";
		
		ShaderProgram::updateValuesInBufferData(prefix + "origin", glm::value_ptr(tree->m_branchesIndexed[i]->origin), 3, info, data);
		ShaderProgram::updateValuesInBufferData(prefix + "phase", &tree->m_branchesIndexed[i]->phase, 1, info, data);
		float pseudoInertiaFactor = 1.0f;
		ShaderProgram::updateValuesInBufferData(prefix + "pseudoInertiaFactor", &pseudoInertiaFactor, 3, info, data);
			
		// orientation is computed from object space direction relative to optimal branch axis
		glm::quat orientation = glm::rotation(glm::vec3(0.0f,1.0f,0.0f), tree->m_branchesIndexed[i]->direction);
		glm::vec4 quatAsVec4 = glm::vec4(orientation.x, orientation.y, orientation.z, orientation.w);
		ShaderProgram::updateValuesInBufferData(prefix + "orientation", glm::value_ptr(quatAsVec4), 4, info, data);
	}
}

TreeAnimation::TreeRendering::TreeRendering()
{
	foliageShader = nullptr;
	branchShader = nullptr;
	branchShadowMapShader = nullptr;
	foliageShadowMapShader = nullptr;
}

TreeAnimation::TreeRendering::~TreeRendering()
{
	delete foliageShader;
	delete branchShader;
	delete foliageShadowMapShader;
	delete branchShadowMapShader;
}

void TreeAnimation::TreeRendering::generateAndConfigureTreeEntities(int numTreeVariants, float treeHeight, float treeWidth, int numMainBranches, int numSubBranches, int numFoliageQuadsPerBranch, const aiScene* branchModel)
{
	treeEntities.resize(numTreeVariants);
	for (int i = 0; i < numTreeVariants; i++)
	{
		treeEntities[i] = new TreeEntity;

		// generate a tree
		TreeAnimation::Tree* tree = TreeAnimation::Tree::generateTree(treeHeight, treeWidth, numMainBranches, numSubBranches);
		treeEntities[i]->tree = tree;
		
		TreeAnimation::FoliageVertexData fData;
		TreeAnimation::BranchesVertexData bData;
		
		TreeAnimation::generateBranchVertexData(&tree->m_trunk, bData, branchModel);
		for (auto b : tree->m_trunk.children)
		{
			TreeAnimation::generateBranchVertexData(b, bData, branchModel);
			TreeAnimation::generateFoliageGeometryShaderVertexData(b, numFoliageQuadsPerBranch, fData);
			for ( auto c : b->children)
			{
				TreeAnimation::generateFoliageGeometryShaderVertexData(c, numFoliageQuadsPerBranch, fData);
				TreeAnimation::generateBranchVertexData(c, bData, branchModel);
			}
		}

		if ( !fData.positions.empty())
		{
			auto fRender =TreeAnimation::generateFoliageGeometryShaderRenderable(fData);

			treeEntities[i]->foliageRenderables.push_back(fRender);
		}

		auto bRender = TreeAnimation::generateBranchesRenderable(bData);
		treeEntities[i]->branchRenderables.push_back(bRender);
	}
}

namespace{
float randFloat(float min, float max) //!< returns a random number between min and max
{
	return (((float) rand() / (float) RAND_MAX) * (max - min) + min); 
}
}

void TreeAnimation::TreeRendering::generateModelMatrices(int numTreesPerTreeVariant, float xMin, float xMax, float zMin, float zMax)
{
	modelMatrices.resize(treeEntities.size());
	for ( unsigned int k = 0; k < modelMatrices.size(); k++)
	{
		modelMatrices[k].resize(numTreesPerTreeVariant);

		for (int i = 0; i < modelMatrices[k].size(); i++)
		{
			// generate random position on x/z plane
			float x = randFloat(xMin, xMax);
			float z = randFloat(zMin, zMax);
			//float y = randFloat(-5.0f, 5.0f);

			float y = 0.0f;
			float rRotY = randFloat(-glm::pi<float>(),glm::pi<float>() );
			float rScaleY = randFloat(0.75f, 1.25f);
			modelMatrices[k][i] = glm::mat4(1.0f);
			modelMatrices[k][i] = glm::scale(glm::vec3(1.0f, rScaleY, 1.0f)) * modelMatrices[k][i];
			modelMatrices[k][i] = glm::rotate(rRotY, glm::vec3(0.0f, 1.0f, 0.0f))* modelMatrices[k][i];
			modelMatrices[k][i] = glm::translate(glm::vec3(x, y, z)) * modelMatrices[k][i];
		}
	}
}

#include <Rendering/GLTools.h>
void TreeAnimation::TreeRendering::createInstanceMatrixAttributes(int attributeLocation)
{
	if ( treeEntities.empty()){ DEBUGLOG->log("ERROR: Create TreeEntities first!"); return;}
	if ( treeEntities.size() != modelMatrices.size()){ DEBUGLOG->log("ERROR: Create model matrices first!"); return;}

	// generate Instance-buffer for renderables of this treeVariant
	auto mat4VertexAttribute = [&](Renderable*r, int attributeLocation)
	{
		r->bind();
		// mat4 Vertex Attribute == 4 x vec4 attributes (consecutively)
		GLsizei vec4Size = sizeof(glm::vec4);
		glEnableVertexAttribArray(attributeLocation); 
		glVertexAttribPointer(attributeLocation, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (GLvoid*)0); // offset = 0 x vec4 size , stride = 4x vec4 size
		glEnableVertexAttribArray(attributeLocation+1); 
		glVertexAttribPointer(attributeLocation+1, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (GLvoid*)(vec4Size)); //offset = 1 x vec4 size
		glEnableVertexAttribArray(attributeLocation+2); 
		glVertexAttribPointer(attributeLocation+2, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (GLvoid*)(2 * vec4Size)); //offset = 2 x vec4 size
		glEnableVertexAttribArray(attributeLocation+3); 
		glVertexAttribPointer(attributeLocation+3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (GLvoid*)(3 * vec4Size)); // offset = 2x vec4 size

		// enable instanced attribute processing
		glVertexAttribDivisor(attributeLocation,   1);
		glVertexAttribDivisor(attributeLocation+1, 1);
		glVertexAttribDivisor(attributeLocation+2, 1);
		glVertexAttribDivisor(attributeLocation+3, 1);
		r->unbind();
	};

	// create vbo from treemodelmatrices and assing to all renderables
	unsigned int i = 0;
	for ( auto t : treeEntities)
	{
		GLuint instanceModelBufferHandle = bufferData<glm::mat4>(modelMatrices[i], GL_STATIC_DRAW);	
	
		for ( auto b : t->branchRenderables) {
			mat4VertexAttribute(b, attributeLocation);
		}
		for ( auto f : t->foliageRenderables) {
			mat4VertexAttribute(f, attributeLocation);
		}
		i++;
	}
}

void TreeAnimation::TreeRendering::createAndConfigureShaders(std::string branchFragmentShader, std::string foliageFragmentShader)
{
	branchShader = new ShaderProgram("/treeAnim/tree.vert", branchFragmentShader);
	foliageShader = new ShaderProgram("/treeAnim/tree.vert", foliageFragmentShader , "/treeAnim/foliage.geom" );
	branchShadowMapShader = new ShaderProgram("/treeAnim/tree.vert", "/vml/shadowmap.frag" );
	foliageShadowMapShader = new ShaderProgram("/treeAnim/tree.vert", "/vml/shadowmap.frag", "/treeAnim/foliage.geom" );
}

void TreeAnimation::TreeRendering::createAndConfigureUniformBlocksAndBuffers(int firstBindingPointIdx)
{
	if ( branchShader == nullptr || foliageShader == nullptr){ DEBUGLOG->log("ERROR: Create shaders first!"); return;}
	if ( treeEntities.empty()){ DEBUGLOG->log("WARNING: Create TreeEntities first, before creating Uniform Block Buffers! No Data will be buffered.");}

	branchShaderUniformBlockInfoMap = ShaderProgram::getAllUniformBlockInfo(*branchShader);
	foliageShaderUniformBlockInfoMap = ShaderProgram::getAllUniformBlockInfo(*foliageShader);
	foliageShadowMapShaderUniformBlockInfoMap = ShaderProgram::getAllUniformBlockInfo(*branchShadowMapShader);
	branchShadowMapShaderUniformBlockInfoMap = ShaderProgram::getAllUniformBlockInfo(*foliageShadowMapShader);

	if (branchShaderUniformBlockInfoMap.find("Tree") == branchShaderUniformBlockInfoMap.end() || foliageShaderUniformBlockInfoMap.find("Tree") == foliageShaderUniformBlockInfoMap.end() ) {
		DEBUGLOG->log("ERROR: At least one Shader has no Uniform Block called 'Tree'"); return;}; 
	if (branchShaderUniformBlockInfoMap.find("Simulation") == branchShaderUniformBlockInfoMap.end() || foliageShaderUniformBlockInfoMap.find("Simulation") == foliageShaderUniformBlockInfoMap.end() ) {
		DEBUGLOG->log("ERROR: At least one Shader has no Uniform Block called 'Simulation'"); return;};

	simulationUniformBlockInfo = branchShaderUniformBlockInfoMap.at("Simulation");
	treeUniformBlockInfo	   = branchShaderUniformBlockInfoMap.at("Tree");

	treeUniformBlockBuffers.resize(treeEntities.size());
	treeBufferDataVectors.resize(treeEntities.size());
	
	simulationBufferDataVector = ShaderProgram::createUniformBlockDataVector(simulationUniformBlockInfo);
	TreeAnimation::updateSimulationUniformsInBufferData(simulationProperties, simulationUniformBlockInfo, simulationBufferDataVector);
	simulationUniformBlockBuffer = ShaderProgram::createUniformBlockBuffer(simulationBufferDataVector, firstBindingPointIdx);

	for ( unsigned int i = 0 ; i < treeEntities.size(); i++)
	{
		treeBufferDataVectors[i] = ShaderProgram::createUniformBlockDataVector( treeUniformBlockInfo );
		TreeAnimation::updateTreeUniformsInBufferData(treeEntities[i]->tree, treeUniformBlockInfo, treeBufferDataVectors[i]);

		treeUniformBlockBuffers[i] = ShaderProgram::createUniformBlockBuffer( treeBufferDataVectors[i], i+(firstBindingPointIdx+1)); // 2..
	}

	// initial binding points
	glUniformBlockBinding(branchShader->getShaderProgramHandle(), branchShaderUniformBlockInfoMap["Simulation"].index, firstBindingPointIdx);
	glUniformBlockBinding(branchShader->getShaderProgramHandle(), branchShaderUniformBlockInfoMap["Tree"].index, firstBindingPointIdx+1);

	glUniformBlockBinding(foliageShader->getShaderProgramHandle(), foliageShaderUniformBlockInfoMap["Simulation"].index, firstBindingPointIdx);
	glUniformBlockBinding(foliageShader->getShaderProgramHandle(), foliageShaderUniformBlockInfoMap["Tree"].index, firstBindingPointIdx+1);

	glUniformBlockBinding(foliageShadowMapShader->getShaderProgramHandle(), foliageShaderUniformBlockInfoMap["Simulation"].index, firstBindingPointIdx);
	glUniformBlockBinding(foliageShadowMapShader->getShaderProgramHandle(), foliageShaderUniformBlockInfoMap["Tree"].index, firstBindingPointIdx+1);

	glUniformBlockBinding(branchShadowMapShader->getShaderProgramHandle(), foliageShaderUniformBlockInfoMap["Simulation"].index, firstBindingPointIdx);
	glUniformBlockBinding(branchShadowMapShader->getShaderProgramHandle(), foliageShaderUniformBlockInfoMap["Tree"].index, firstBindingPointIdx+1);
}


void TreeAnimation::TreeRendering::createAndConfigureRenderpasses(FrameBufferObject* targetBranchFBO, FrameBufferObject* targetFoliageFBO, FrameBufferObject* targetShadowMapFBO)
{
	if ( treeEntities.empty()){ DEBUGLOG->log("ERROR: Create TreeEntities first!"); return;}
	if ( branchShader == nullptr || foliageShader == nullptr){ DEBUGLOG->log("ERROR: Create shaders first!"); return;}

	branchRenderpasses.resize(treeEntities.size());
	foliageRenderpasses.resize(treeEntities.size());
	branchShadowMapRenderpasses.resize(treeEntities.size());
	foliageShadowMapRenderpasses.resize(treeEntities.size());
	for ( int i = 0; i < treeEntities.size(); i++)
	{
		branchRenderpasses[i] = new RenderPass(branchShader, targetBranchFBO);
		for ( auto r : treeEntities[i]->branchRenderables )
		{
			branchRenderpasses[i]->addRenderable(r);
		}
		branchRenderpasses[i]->addEnable(GL_DEPTH_TEST);

		foliageRenderpasses[i] = new RenderPass(foliageShader, targetFoliageFBO);
		for ( auto r : treeEntities[i]->foliageRenderables )
		{
			foliageRenderpasses[i]->addRenderable(r);
		}
		foliageRenderpasses[i]->addEnable(GL_ALPHA_TEST); // for foliage
		foliageRenderpasses[i]->addEnable(GL_DEPTH_TEST);

		if ( targetShadowMapFBO != nullptr)
		{
			// SHADOW MAP
			branchShadowMapRenderpasses[i] = new RenderPass(branchShadowMapShader, targetShadowMapFBO);
			for ( auto r : treeEntities[i]->branchRenderables )
			{
				branchShadowMapRenderpasses[i]->addRenderable(r);
			}
			branchShadowMapRenderpasses[i]->addEnable(GL_DEPTH_TEST);

			foliageShadowMapRenderpasses[i] = new RenderPass(foliageShadowMapShader, targetShadowMapFBO);
			for ( auto r : treeEntities[i]->foliageRenderables )
			{
				foliageShadowMapRenderpasses[i]->addRenderable(r);
			}
			//foliageShadowMapRenderpasses[i]->addEnable(GL_ALPHA_TEST); // for foliage
			foliageShadowMapRenderpasses[i]->addEnable(GL_DEPTH_TEST);
		}
	}
	glAlphaFunc(GL_GREATER, 0);
}

#include <UI/imgui/imgui.h>
namespace{
static bool updateAngleShifts = false;
static bool updateAmplitudes = false;
static bool updateFrequencies = false;
}
void TreeAnimation::TreeRendering::imguiInterfaceSimulationProperties()
{
	if (ImGui::CollapsingHeader("Angle Shifts"))
	{   ImGui::SliderFloat3("vAngleShiftFront", glm::value_ptr( simulationProperties.angleshifts[0]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAngleShiftBack", glm::value_ptr(  simulationProperties.angleshifts[1]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAngleShiftSide", glm::value_ptr(  simulationProperties.angleshifts[2]), -1.0f, 1.0f);
		updateAngleShifts = true;
	}else {updateAngleShifts = false;}
		
	if (ImGui::CollapsingHeader("Amplitudes"))
	{   ImGui::SliderFloat3("vAmplitudesFront", glm::value_ptr( simulationProperties.amplitudes[0]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAmplitudesBack", glm::value_ptr(  simulationProperties.amplitudes[1]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAmplitudesSide", glm::value_ptr(  simulationProperties.amplitudes[2]), -1.0f, 1.0f); 
		updateAmplitudes = true;
	}else{updateAmplitudes = false;}


	if (ImGui::CollapsingHeader("Frequencies"))
	{   ImGui::SliderFloat3("fFrequencies", glm::value_ptr( simulationProperties.frequencies), 0.0f, 3.0f);
		updateFrequencies = true;
	}else{ updateFrequencies =false; }
}
void TreeAnimation::TreeRendering::updateActiveImguiInterfaces()
{
	if (updateAngleShifts){
		ShaderProgram::updateValueInBuffer("vAngleShiftFront", glm::value_ptr(simulationProperties.angleshifts[0]),3, simulationUniformBlockInfo, simulationUniformBlockBuffer); //front
		ShaderProgram::updateValueInBuffer("vAngleShiftBack", glm::value_ptr(simulationProperties.angleshifts[1]),3, simulationUniformBlockInfo, simulationUniformBlockBuffer); // back
		ShaderProgram::updateValueInBuffer("vAngleShiftSide", glm::value_ptr(simulationProperties.angleshifts[2]),3, simulationUniformBlockInfo, simulationUniformBlockBuffer);} //side
			
	if (updateAmplitudes){
		ShaderProgram::updateValueInBuffer("vAmplitudesFront", glm::value_ptr(simulationProperties.amplitudes[0]),3,simulationUniformBlockInfo, simulationUniformBlockBuffer); //front
		ShaderProgram::updateValueInBuffer("vAmplitudesBack", glm::value_ptr(simulationProperties.amplitudes[1]),3, simulationUniformBlockInfo, simulationUniformBlockBuffer); // back
		ShaderProgram::updateValueInBuffer("vAmplitudesSide", glm::value_ptr(simulationProperties.amplitudes[2]),3, simulationUniformBlockInfo, simulationUniformBlockBuffer);} //side
		
	if (updateFrequencies){
		ShaderProgram::updateValueInBuffer("fFrequencyFront", &simulationProperties.frequencies.x,1,simulationUniformBlockInfo, simulationUniformBlockBuffer); //front
		ShaderProgram::updateValueInBuffer("fFrequencyBack", &simulationProperties.frequencies.y,1, simulationUniformBlockInfo, simulationUniformBlockBuffer); // back
		ShaderProgram::updateValueInBuffer("fFrequencySide", &simulationProperties.frequencies.z,1, simulationUniformBlockInfo, simulationUniformBlockBuffer);} //side
}
