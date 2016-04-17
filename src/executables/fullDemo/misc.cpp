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

#include <glm/gtc/type_ptr.hpp>
#include <Rendering/PostProcessing.h>
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
static const int NUM_TREES_PER_VARIANT = 20;
static const int NUM_FOLIAGE_QUADS_PER_BRANCH = 5;
static Assimp::Importer branchImporter;
static Assimp::Importer trunkImporter;
static std::vector<std::map<aiTextureType, GLuint>> s_tree_materials_textures; //!< mapping material texture types to texture handles
static std::vector<AssimpTools::MaterialInfo> s_tree_material_infos; //!< mapping material texture types to texture handles
static const glm::vec4 FORESTED_AREA = glm::vec4(-20.0f,-20.0f, 20.0f,20.0f);
inline void loadBranchModel()
{
	std::string trunkModel = "branch_detailed.dae";
	std::string branchModel = "branch_simple.dae";
	const aiScene* trunkScene = AssimpTools::importAssetFromResourceFolder(trunkModel, trunkImporter);
	const aiScene* branchScene = AssimpTools::importAssetFromResourceFolder(branchModel, branchImporter);
	std::map<aiTextureType, AssimpTools::MaterialTextureInfo> branchTexturesInfo;
	AssimpTools::MaterialInfo branchMaterialInfo = AssimpTools::getMaterialInfo(trunkScene, 0);
	s_tree_material_infos.push_back(branchMaterialInfo);
	if (trunkScene != NULL) branchTexturesInfo = AssimpTools::getMaterialTexturesInfo(trunkScene, 0);
	if (trunkScene != NULL) s_tree_materials_textures.resize(trunkScene->mNumMaterials);

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
	OPENGLCONTEXT->bindTexture(foliageTexHandle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	OPENGLCONTEXT->bindTexture(0);

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
		trunkImporter.GetScene(),
		branchImporter.GetScene()
		);

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
inline void assignHeightMapUniforms(TreeAnimation::TreeRendering& treeRendering, GLuint distortionTex, glm::vec4 terrainRange)
{
	treeRendering.branchShader->bindTextureOnUse("heightMap", distortionTex);
	treeRendering.branchShader->update("heightMapRange", terrainRange); // grass size
	treeRendering.foliageShader->bindTextureOnUse("heightMap", distortionTex);
	treeRendering.foliageShader->update("heightMapRange", terrainRange); // grass size
	treeRendering.branchShadowMapShader->bindTextureOnUse("heightMap", distortionTex);
	treeRendering.branchShadowMapShader->update("heightMapRange", terrainRange); // grass size
	treeRendering.foliageShadowMapShader->bindTextureOnUse("heightMap", distortionTex);
	treeRendering.foliageShadowMapShader->update("heightMapRange", terrainRange); // grass size
}
inline void animateSeasons(TreeAnimation::TreeRendering& treeRendering, ShaderProgram& sh_grassGeom, float t, float& grassSize, float& windPower, float& foliageSize,ShaderProgram& sh_tessellation )
{
	foliageSize = max<float>( 0.0f, sin(t - glm::half_pi<float>())) * 0.5f; // 0.0 for most of the time, begins to grow at 0.5 pi, goes away at 1.5pi
	grassSize = foliageSize + 0.2f - abs(sin((t + glm::half_pi<float>())) * 0.2f);
	windPower = 0.5f + 0.3f*(cos( t + glm::quarter_pi<float>())); // always > 0 , becomes angry at 1.5 pi
	float snowBegin = 0.6 + (min(1.0f,foliageSize) - 1.0f ) * 0.35f;
	float stonesEnd = 0.5 + (min(1.0f,foliageSize) - 1.0f ) * 0.35f;
	float grassEnd = 0.2 + (min(1.0f,foliageSize) - 1.0f ) * 0.1f;
	float grassBegin = 0.1+ (min(1.0f,foliageSize) - 1.0f ) * 0.1f;
	sh_tessellation.update("heightZones", glm::vec4(grassBegin,grassEnd, stonesEnd, snowBegin));
}

/***********************************************/


inline void updateLightCamera(Camera& mainCamera, Camera& lightSourceCamera, glm::vec3 offset)
{
	lightSourceCamera.setPosition( mainCamera.getPosition() + offset);
	//TODO move lightSourceCamera according to mainCamera
}

static bool s_dynamicDoF = false;
inline void imguiDynamicFieldOfView(PostProcessing::DepthOfField& r_depthOfField)
{
	ImGui::Checkbox("dynamic DoF", &s_dynamicDoF);
}

inline void updateDynamicFieldOfView(PostProcessing::DepthOfField& r_depthOfField, FrameBufferObject& gbufferFBO, float dt)
{
	if ( s_dynamicDoF )
	{
		// read center depth
		gbufferFBO.bind();
		glm::vec4 value;
		glReadBuffer(GL_COLOR_ATTACHMENT2); // position buffer
		glReadPixels(gbufferFBO.getWidth() / 2, gbufferFBO.getHeight() /2, 1, 1,
			GL_RGBA ,GL_FLOAT, glm::value_ptr(value) );
		float depth = glm::length(glm::vec3(value));

		float diffNear = (depth / 2.0f) - r_depthOfField.m_focusPlaneDepths.y;
		float diffFar = (depth + depth/2.0f) - r_depthOfField.m_focusPlaneDepths.z;

		r_depthOfField.m_focusPlaneDepths.x = min(r_depthOfField.m_focusPlaneDepths.x + diffNear * ((abs(diffNear) > 50.0f) ? 0.9f : dt) , 30.0f);
		r_depthOfField.m_focusPlaneDepths.y = min( r_depthOfField.m_focusPlaneDepths.y + diffNear * ((abs(diffNear) > 50.0f) ? 0.9f : dt), 40.0f);
		r_depthOfField.m_focusPlaneDepths.z = r_depthOfField.m_focusPlaneDepths.z + diffFar * ((abs(diffFar) > 50.0f) ? 0.9f : dt);
		r_depthOfField.m_focusPlaneDepths.w = r_depthOfField.m_focusPlaneDepths.w + diffFar * ((abs(diffFar) > 50.0f) ? 0.9f : dt);

		r_depthOfField.updateUniforms();
	}
}

#endif