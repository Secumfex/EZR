#ifndef TREERENDERING_H
#define TREERENDERING_H

#include <glm/glm.hpp>
#include <vector>

#include "Tree.h"
#include <Rendering/VertexArrayObjects.h>
#include <Rendering/ShaderProgram.h>

class aiScene;

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


void updateTreeUniforms(ShaderProgram& shaderProgram, TreeAnimation::Tree* tree);

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
void updateSimulationUniforms(ShaderProgram& shaderProgram, TreeAnimation::SimulationProperties& simulation);

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


} // TreeAnimation
#endif