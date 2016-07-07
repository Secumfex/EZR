#ifndef TREERENDERING_H
#define TREERENDERING_H

#include <glm/glm.hpp>
#include <vector>

#include "Tree.h"
#include <Rendering/RenderPass.h>
#include <assimp/Importer.hpp>

namespace TreeAnimation
{

Renderable* generateRenderable(TreeAnimation::Tree::Branch* branch, const aiScene* branchModel = NULL);

Renderable* generateFoliage(TreeAnimation::Tree::Branch* branch, int numLeafs, const aiScene* foliageModel = NULL);

struct FoliageVertexData
{
	std::vector<float> positions;
	std::vector<unsigned int> indices;
	std::vector<float> normals;
	std::vector<float> uvs;
	std::vector<unsigned int> hierarchy;
};

void generateFoliageVertexData(TreeAnimation::Tree::Branch* branch, int numLeafs, FoliageVertexData& target);
Renderable* generateFoliageRenderable(FoliageVertexData& source); // use this source to generate a single renderable

void generateFoliageGeometryShaderVertexData(TreeAnimation::Tree::Branch* branch, int numLeafs, FoliageVertexData& target); //!< use this for geometry shader
Renderable* generateFoliageGeometryShaderRenderable(FoliageVertexData& source); // use this source to generate a single renderable suitable for a geometry shader

void updateTreeUniforms(ShaderProgram& shaderProgram, TreeAnimation::Tree* tree);
void updateTreeUniformsInBufferData(Tree* tree, ShaderProgram::UniformBlockInfo& info, std::vector<float>& data);

struct SimulationProperties
{
	glm::vec3 angleshifts[3];
	glm::vec3 amplitudes[3];
	glm::vec3 frequencies;
	SimulationProperties()
		: frequencies(2.5f) {
		amplitudes[0] = glm::vec3(0.3f);
		amplitudes[1] = glm::vec3(0.3f);
		amplitudes[2] = glm::vec3(0.3f);
		angleshifts[0] = glm::vec3(0.0);
		angleshifts[1] = glm::vec3(0.0);
		angleshifts[2] = glm::vec3(0.0);
	}
};
void updateSimulationUniforms(ShaderProgram& shaderProgram, SimulationProperties& simulation);
void updateSimulationUniformsInBufferData(SimulationProperties& simulation, ShaderProgram::UniformBlockInfo& info,  std::vector<float>& data);

struct BranchesVertexData
{
	std::vector<float> positions;
	std::vector<unsigned int> indices;
	std::vector<float> normals;
	std::vector<float> uvs;
	std::vector<float> tangents;
	std::vector<unsigned int> hierarchy;
};

void generateBranchVertexData(TreeAnimation::Tree::Branch* branch, BranchesVertexData& target, const aiScene* scene = NULL);
Renderable* generateBranchesRenderable(BranchesVertexData& source); // use this source to generate a single renderable

struct TreeEntity { 
	TreeAnimation::Tree* tree;
	std::vector< Renderable* > branchRenderables;
	std::vector< Renderable* > foliageRenderables;
};

class TreeRendering
{
public:

	std::vector<TreeEntity* > treeEntities;
	std::vector<std::vector<glm::mat4>> modelMatrices;

	ShaderProgram* foliageShader;
	ShaderProgram* branchShader;
	ShaderProgram* branchShadowMapShader;
	ShaderProgram* foliageShadowMapShader;

	std::vector<RenderPass* > foliageRenderpasses;
	std::vector<RenderPass* > branchRenderpasses;
	std::vector<RenderPass* > foliageShadowMapRenderpasses;
	std::vector<RenderPass* > branchShadowMapRenderpasses;

	ShaderProgram::UniformBlockInfo treeUniformBlockInfo;
	ShaderProgram::UniformBlockInfo simulationUniformBlockInfo;
	std::unordered_map<std::string, ShaderProgram::UniformBlockInfo> branchShaderUniformBlockInfoMap;
	std::unordered_map<std::string, ShaderProgram::UniformBlockInfo> foliageShaderUniformBlockInfoMap;
	std::unordered_map<std::string, ShaderProgram::UniformBlockInfo> foliageShadowMapShaderUniformBlockInfoMap;
	std::unordered_map<std::string, ShaderProgram::UniformBlockInfo> branchShadowMapShaderUniformBlockInfoMap;

	std::vector<GLuint> treeUniformBlockBuffers;
	GLuint simulationUniformBlockBuffer;

	std::vector<std::vector<float>> treeBufferDataVectors;
	std::vector<float> simulationBufferDataVector;

	SimulationProperties simulationProperties;

	TreeRendering();
	~TreeRendering();
	void generateAndConfigureTreeEntities(int numTreeVariants, float treeHeight, float treeWidth, int numMainBranches, int numSubBranches, int numFoliageQuadsPerBranch,  const aiScene* trunkModel, const aiScene* branchModel);
	void generateModelMatrices(int numTreesPerTreeVariant, float xMin, float xMax, float zMin, float zMax);
	void createInstanceMatrixAttributes(int attributeLocation = 5);
	void createAndConfigureShaders(std::string branchFragmentShader = "/modelSpace/GBuffer.frag", std::string foliageFragmentShader = "/treeAnim/foliage.frag");
	void createAndConfigureUniformBlocksAndBuffers(int firstBindingPointIdx = 1);
	void createAndConfigureRenderpasses(FrameBufferObject* targetBranchFBO, FrameBufferObject* targetFoliageFBO, FrameBufferObject* targetShadowMapFBO = nullptr);

	// Imgui
	void imguiInterfaceSimulationProperties();
	void updateActiveImguiInterfaces();
};

} // namespace TreeAnimation
#endif