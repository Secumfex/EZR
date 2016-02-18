#ifndef MISC_CPP
#define MISC_CPP

#include "UI/imgui/imgui.h"
#include <UI/imguiTools.h>
#include <functional>
#include <Core/Camera.h>
#include <Rendering/GLTools.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <Importing/AssimpTools.h>

#include <Importing/TextureTools.h>

#include <TreeAnimation/Tree.h>
#include <TreeAnimation/TreeRendering.h>
#include <TreeAnimation/WindField.h>

/***********************************************/
// This file is for arbitrary stuff to save some ugly Lines of Code
/***********************************************/

/***********************************************/
namespace CallbackHelper
{
	static double old_x;
	static double old_y;
	static double d_x;
	static double d_y;

	static std::function<void (double, double)> cursorPosFunc;
	static std::function<void (int, int, int)> mouseButtonFunc;
	static std::function<void (int,int,int,int)> keyboardFunc;

	static bool active_mouse_control = false;

	static Camera* mainCamera = nullptr;
	static GLFWwindow* window;

	inline void cursorPosCallback(double x, double y)
	{
		ImGuiIO& io = ImGui::GetIO();
	 	if ( io.WantCaptureMouse )
	 	{ return; } // ImGUI is handling this

		d_x = x - old_x;
		d_y = y - old_y;

		if ( active_mouse_control  && mainCamera != nullptr)
		{
			mainCamera->mouseControlCallback(d_y, d_x);
		}

		cursorPosFunc(x,y); // inner stuff from main()

		old_x = x;
		old_y = y;
	}

	inline void mouseButtonCallback(int b, int a, int m)
	{
		if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_PRESS)
		{
			active_mouse_control = true;
		}
		if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_RELEASE)
		{
			active_mouse_control = false;
		}

		mouseButtonFunc(b, a, m);

	 	ImGui_ImplGlfwGL3_MouseButtonCallback(window, b, a, m);
	}

	inline void keyboardCallback(int k, int s, int a, int m)
	{
		mainCamera->keyboardControlCallback(k,s,a,m);

		keyboardFunc(k,s,a,m);

		ImGui_ImplGlfwGL3_KeyCallback(window,k,s,a,m);
 	}

	inline void setupCallbackHelper(GLFWwindow* window, Camera* mainCam)
	{
	    ImGui_ImplGlfwGL3_Init(window, true);
		
		glfwGetCursorPos(window, &old_x, &old_y);
		
		mainCamera = mainCam;
		
		CallbackHelper::window = window;

		setCursorPosCallback(window, cursorPosCallback);
		setMouseButtonCallback(window, mouseButtonCallback);
		setKeyCallback(window, keyboardCallback);
	}
}
/***********************************************/

/*****************TREE ANIMATION**********************/
static const float TREE_HEIGHT = 4.0f;
static const float TREE_WIDTH = TREE_HEIGHT / 10.0f;
static const int NUM_MAIN_BRANCHES = 5;
static const int NUM_SUB_BRANCHES  = 3;
static const int NUM_TREE_VARIANTS = 3;
static const int NUM_TREES_PER_VARIANT = 10;
static const int NUM_FOLIAGE_QUADS_PER_BRANCH = 5;
static Assimp::Importer branchImporter;
static std::vector<std::map<aiTextureType, GLuint>> s_tree_materials_textures; //!< mapping material texture types to texture handles
static std::vector<AssimpTools::MaterialInfo> s_tree_material_infos; //!< mapping material texture types to texture handles
static const glm::vec4 FORESTED_AREA = glm::vec4(-15.0f,-15.0f, 15.0f,15.0f);
inline void loadBranchModel()
{
	std::string branchModel = "branch_detailed.dae";
	const aiScene* branchScene = AssimpTools::importAssetFromResourceFolder(branchModel, branchImporter);
	std::map<aiTextureType, AssimpTools::MaterialTextureInfo> branchTexturesInfo;
	AssimpTools::MaterialInfo branchMaterialInfo = AssimpTools::getMaterialInfo(branchScene, 0);
	s_tree_material_infos.push_back(branchMaterialInfo);
	if (branchScene != NULL) branchTexturesInfo = AssimpTools::getMaterialTexturesInfo(branchScene, 0);
	if (branchScene != NULL) s_tree_materials_textures.resize(branchScene->mNumMaterials);

	for (auto e : branchTexturesInfo)
	{
		GLuint texHandle = TextureTools::loadTextureFromResourceFolder(branchTexturesInfo[e.first].relativePath);
		if (texHandle != -1){ s_tree_materials_textures[e.second.matIdx][e.first] = texHandle; }
	}
}

inline void loadFoliageMaterial()
{
	std::string foliageTexture = "foliage_texture.png";
	auto foliageTexHandle = TextureTools::loadTextureFromResourceFolder(foliageTexture);
	std::map<aiTextureType, GLuint > foliageMatTextures;
	foliageMatTextures[aiTextureType_DIFFUSE] = foliageTexHandle;
	s_tree_materials_textures.push_back(foliageMatTextures);
	glBindTexture(GL_TEXTURE_2D, foliageTexHandle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

}

inline void generateTrees(TreeAnimation::TreeRendering& treeRendering)
{
	DEBUGLOG->log("Setup: generating trees"); DEBUGLOG->indent();

	// generate a forest randomly, including renderables
	treeRendering.generateAndConfigureTreeEntities(
		NUM_TREE_VARIANTS,
		TREE_HEIGHT, TREE_WIDTH,
		NUM_MAIN_BRANCHES, NUM_SUB_BRANCHES,
		NUM_FOLIAGE_QUADS_PER_BRANCH,
		branchImporter.GetScene());

	treeRendering.generateModelMatrices(
		NUM_TREES_PER_VARIANT,
		FORESTED_AREA.x, FORESTED_AREA.z, FORESTED_AREA.y, FORESTED_AREA.w);

	treeRendering.createInstanceMatrixAttributes();
	DEBUGLOG->outdent();
}
inline void assignTreeMaterialTextures(TreeAnimation::TreeRendering& treeRendering)
{
	if (! s_tree_materials_textures.empty()){
	auto difftex = s_tree_materials_textures[0].find(aiTextureType_DIFFUSE);
	if ( difftex != s_tree_materials_textures[0].end())
	{
		treeRendering.branchShader->bindTextureOnUse("tex", difftex->second);
		treeRendering.branchShader->update("mixTexture", 1.0f);
	}
	auto normaltex = s_tree_materials_textures[0].find(aiTextureType_NORMALS);
	if (normaltex != s_tree_materials_textures[0].end())
	{
		treeRendering.branchShader->bindTextureOnUse("normalTex", normaltex->second);
		treeRendering.branchShader->update("hasNormalTex", true);
	}}

	GLuint foliageTexHandle = s_tree_materials_textures[1].find(aiTextureType_DIFFUSE)->second;
	treeRendering.foliageShader->bindTextureOnUse("tex", foliageTexHandle);

	if (s_tree_material_infos.empty()) return;

	auto branch_shininess = s_tree_material_infos[0].scalar.find(AssimpTools::SHININESS);
	auto branch_shininess_strength = s_tree_material_infos[0].scalar.find(AssimpTools::SHININESS_STRENGTH);
	if ( !s_tree_material_infos.empty() &&  branch_shininess != s_tree_material_infos[0].scalar.end() && branch_shininess_strength != s_tree_material_infos[0].scalar.end())
	{
		treeRendering.branchShader->update("shininess", branch_shininess->second);
		treeRendering.branchShader->update("shininess_strength", branch_shininess_strength->second);
	}
	else
	{
		treeRendering.branchShader->update("shininess", 10.0f);
		treeRendering.branchShader->update("shininess_strength", 0.2f);
	}
	treeRendering.branchShader->update("materialType", 0.0);
}
inline void assignWindFieldUniforms(TreeAnimation::TreeRendering& treeRendering, TreeAnimation::WindField& windField)
{
	// windfield
	treeRendering.branchShader->bindTextureOnUse( "windField", windField.m_vectorTextureHandle);
	treeRendering.foliageShader->bindTextureOnUse("windField", windField.m_vectorTextureHandle);
	treeRendering.branchShader->update( "windFieldArea", FORESTED_AREA);
	treeRendering.foliageShader->update("windFieldArea", FORESTED_AREA);

	treeRendering.branchShadowMapShader->bindTextureOnUse( "windField", windField.m_vectorTextureHandle);
	treeRendering.foliageShadowMapShader->bindTextureOnUse("windField", windField.m_vectorTextureHandle);
	treeRendering.branchShadowMapShader->update( "windFieldArea", FORESTED_AREA);
	treeRendering.foliageShadowMapShader->update("windFieldArea", FORESTED_AREA);
}
/***********************************************/


inline void updateLightCamera(Camera& mainCamera, Camera& lightSourceCamera, glm::vec3 offset)
{
	lightSourceCamera.setPosition( mainCamera.getPosition() + offset);
	//TODO move lightSourceCamera according to mainCamera
}

#endif