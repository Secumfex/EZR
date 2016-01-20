/*******************************************
 * **** DESCRIPTION ****
 ****************************************/

#include <iostream>
#include <time.h>

#include <Rendering/GLTools.h>
#include <Rendering/VertexArrayObjects.h>
#include <Rendering/RenderPass.h>

#include "UI/imgui/imgui.h"
#include <UI/imguiTools.h>
#include <UI/Turntable.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <Importing/AssimpTools.h>
#include <Importing/TextureTools.h>

#include <TreeAnimation/Tree.h>
#include <TreeAnimation/TreeRendering.h>
#include <TreeAnimation/WindField.h>

////////////////////// PARAMETERS /////////////////////////////
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);
const float TREE_HEIGHT = 4.0f;
const float TREE_WIDTH = TREE_HEIGHT / 10.0f;
const int NUM_MAIN_BRANCHES = 3;
const int NUM_SUB_BRANCHES  = 7;
const int NUM_TREE_VARIANTS = 3;
const int NUM_TREES_PER_VARIANT = 30;

static glm::vec4 s_trunk_color =  glm::vec4(107.0f / 255.0f , 68.0f / 255.0f , 35.0f /255.0f, 1.0f); // brown
static glm::vec4 s_foliage_color = glm::vec4(22.0f / 255.0f , 111.0f / 255.0f , 22.0f /255.0f, 1.0f); // green
static glm::vec4 s_lightPos = glm::vec4(0.0,50.0f,0.0,1.0);

static float s_wind_angle = 45.0f;
static glm::vec3 s_wind_direction = glm::rotateY(glm::vec3(1.0f,0.0f,0.0f), glm::radians(s_wind_angle));
static glm::mat4 s_wind_rotation = glm::mat4(1.0f);
static float s_wind_power = 1.0f;

static float s_strength = 1.0f;
static bool  s_isRotating = false;

static float s_simulationTime = 0.0f;

static float s_eye_distance = 10.0f;

static std::map<Renderable*, glm::vec4*> s_renderable_color_map;
static std::map<Renderable*, int> s_renderable_material_map; //!< mapping a renderable to a material index
static std::vector<std::map<aiTextureType, GLuint>> s_material_texture_handles; //!< mapping material texture types to texture handles

//////////////////// MISC /////////////////////////////////////
float randFloat(float min, float max) //!< returns a random number between min and max
{
	return (((float) rand() / (float) RAND_MAX) * (max - min) + min); 
}


//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

struct TreeEntity { 
	TreeAnimation::Tree* tree;
	//glm::vec3 position;
	std::vector< Renderable* > branchRenderables;
	std::vector< Renderable* > foliageRenderables;
};

std::vector<glm::mat4 > generateModelMatrices(int numObjects, float xSize, float zSize)
{
	std::vector<glm::mat4> models(numObjects);

	for (int i = 0; i < models.size(); i++)
	{

		// generate random position on x/z plane
		float x = randFloat(-xSize * 0.5f, xSize * 0.5f);
		float z = randFloat(-zSize * 0.5f, zSize * 0.5f);
		//float y = randFloat(-5.0f, 5.0f);
		float y = 0.0f;
		float rRotY = randFloat(-glm::pi<float>(),glm::pi<float>() );
		float rScaleY = randFloat(0.75f, 1.25f);
		models[i] = glm::mat4(1.0f);
		models[i] = glm::scale(glm::vec3(1.0f, rScaleY, 1.0f)) * models[i];
		models[i] = glm::rotate(rRotY, glm::vec3(0.0f, 1.0f, 0.0f))* models[i];
		models[i] = glm::translate(glm::vec3(x, y, z)) * models[i];
	}
	return models;
}

std::vector<TreeEntity* > generateTreeVariants(int numTrees, const aiScene* branchModel = NULL, const aiScene* foliageModel = NULL)
{
	std::vector<TreeEntity* > treeEntities(numTrees);
	for (int i = 0; i < numTrees; i++)
	{
		treeEntities[i] = new TreeEntity;

		// generate a tree
		TreeAnimation::Tree* tree = TreeAnimation::Tree::generateTree(TREE_HEIGHT, TREE_WIDTH, NUM_MAIN_BRANCHES, NUM_SUB_BRANCHES);
		treeEntities[i]->tree = tree;
		
		TreeAnimation::FoliageVertexData fData;
		TreeAnimation::BranchesVertexData bData;
		
		TreeAnimation::generateBranchVertexData(&tree->m_trunk, bData, branchModel);
		for (auto b : tree->m_trunk.children)
		{
			TreeAnimation::generateBranchVertexData(b, bData, branchModel);
			TreeAnimation::generateFoliageVertexData(b, 35, fData);
			for ( auto c : b->children)
			{
				TreeAnimation::generateFoliageVertexData(c, 35, fData);
				TreeAnimation::generateBranchVertexData(c, bData, branchModel);
			}
		}

		if ( !fData.positions.empty())
		{
			auto fRender =TreeAnimation::generateFoliageRenderable(fData);
		
			s_renderable_color_map[fRender] = &s_foliage_color;
			s_renderable_material_map[fRender] = 1;
			treeEntities[i]->foliageRenderables.push_back(fRender);
		}

		auto bRender = TreeAnimation::generateBranchesRenderable(bData);

		s_renderable_color_map[bRender] = &s_trunk_color;
		if(branchModel!= NULL)s_renderable_material_map[bRender] = branchModel->mMeshes[0]->mMaterialIndex;
		treeEntities[i]->branchRenderables.push_back(bRender);
	}
	return treeEntities;
}

// update Tree-Simulation related uniforms
void updateTreeUniforms(ShaderProgram& shaderProgram, TreeEntity& treeEntity)
{	
	TreeAnimation::Tree* tree = treeEntity.tree; 
	shaderProgram.update("tree.phase", tree->m_phase); //front

	auto branches = tree->m_branchesIndexed;

	// upload tree uniforms
	for (unsigned int i = 0; i < branches.size(); i++)
	{
		std::string prefix = "tree.branches[" + DebugLog::to_string(i) + "].";

		shaderProgram.update(prefix + "origin", branches[i]->origin);
		shaderProgram.update(prefix + "phase", branches[i]->phase);	
		shaderProgram.update(prefix + "pseudoInertiaFactor", 1.0f);
			
		// orientation is computed from object space direction relative to optimal branch axis
		glm::quat orientation = glm::rotation(glm::vec3(0.0f,1.0f,0.0f), branches[i]->direction);
		glm::vec4 quatAsVec4 = glm::vec4(orientation.x, orientation.y, orientation.z, orientation.w);
		shaderProgram.update(prefix + "orientation", quatAsVec4);
	}
}

int main()
{
	DEBUGLOG->setAutoPrint(true);
	auto window = generateWindow(WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////// INIT /////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	/////////////////////    load tree related assets   //////////////////////////
	DEBUGLOG->log("Setup: generating renderables"); DEBUGLOG->indent();

	// import using ASSIMP and check for errors
	Assimp::Importer importer;
	DEBUGLOG->log("Loading branch model");
	std::string branchModel = "branch_detailed.dae";
	
	const aiScene* scene = AssimpTools::importAssetFromResourceFolder(branchModel, importer);
	std::map<aiTextureType, AssimpTools::MaterialTextureInfo> branchTexturesInfo;
	if (scene != NULL) branchTexturesInfo = AssimpTools::getMaterialTexturesInfo(scene, 0);
	if (scene != NULL) s_material_texture_handles.resize(scene->mNumMaterials);

	for (auto e : branchTexturesInfo)
	{
		GLuint texHandle = TextureTools::loadTextureFromResourceFolder(branchTexturesInfo[e.first].relativePath);
		if (texHandle != -1){ s_material_texture_handles[e.second.matIdx][e.first] = texHandle; }
	}

	// also add a dummy material info for the foliage
	std::string foliageTexture = "foliage_texture.png";
	auto foliageTexHandle = TextureTools::loadTextureFromResourceFolder(foliageTexture);
	std::map<aiTextureType, GLuint > foliageMatTextures;
	foliageMatTextures[aiTextureType_DIFFUSE] = foliageTexHandle;
	s_material_texture_handles.push_back(foliageMatTextures);

	/////////////////////    create Tree data           //////////////////////////
	DEBUGLOG->log("Setup: generating trees"); DEBUGLOG->indent();
	srand (time(NULL));	

	// generate a forest randomly, including renderables
	std::vector<TreeEntity* > treeVariants = generateTreeVariants(NUM_TREE_VARIANTS, scene);
	std::vector<glm::mat4 > treeModelMatrices = generateModelMatrices(NUM_TREES_PER_VARIANT * NUM_TREE_VARIANTS, 30.0f, 30.0f);

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
	GLuint instanceModelBufferHandle = bufferData<glm::mat4>(treeModelMatrices, GL_STATIC_DRAW);	
	for ( auto t : treeVariants)
	{ 
		GLuint attributeLocation = 5; // beginning attribute location (0..4 are reserved for pos,uv,norm,tangents, tree hierarchy
		for ( auto b : t->branchRenderables) {
			mat4VertexAttribute(b, attributeLocation);
		}
		for ( auto f : t->foliageRenderables) {
			mat4VertexAttribute(f, attributeLocation);
		}
	}

	DEBUGLOG->outdent();

	/////////////////////    create wind field          //////////////////////////
	TreeAnimation::WindField windField(64,64);
	windField.updateVectorTexture(0.0f);

	DEBUGLOG->outdent();
	//////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// RENDERING  ///////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	
	/////////////////////     Scene / View Settings     //////////////////////////
	glm::vec4 eye(0.0f, 0.0, s_eye_distance, 1.0f);
	glm::vec4 center(0.0f,0.0f,0.0f,1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0,1,0));

	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.5f, 100.f);

	/////////////////////// 	Renderpasses     ///////////////////////////
	// regular GBuffer
	DEBUGLOG->log("Shader Compilation: GBuffer"); DEBUGLOG->indent();
	ShaderProgram shaderProgram("/treeAnim/tree.vert", "/modelSpace/GBuffer.frag"); DEBUGLOG->outdent();
	shaderProgram.update("view", view);
	shaderProgram.update("projection", perspective);

	static glm::vec3 angleshifts[3] ={glm::vec3(0.0),glm::vec3(0.0),glm::vec3(0.0)};
	static glm::vec3 amplitudes[3] = {glm::vec3(0.3f),glm::vec3(0.3f),glm::vec3(0.3f)};
	static glm::vec3 frequencies(2.5f);
	shaderProgram.update("vAngleShiftFront", angleshifts[0]); //front
	shaderProgram.update("vAngleShiftBack", angleshifts[1]); //back
	shaderProgram.update("vAngleShiftSide", angleshifts[2]); //side
	shaderProgram.update("vAmplitudesFront", amplitudes[0]); //front
	shaderProgram.update("vAmplitudesBack", amplitudes[1]); //back
	shaderProgram.update("vAmplitudesSide", amplitudes[2]); //side
	shaderProgram.update("fFrequencyFront", frequencies.x); //front
	shaderProgram.update("fFrequencyBack", frequencies.y); //back
	shaderProgram.update("fFrequencySide", frequencies.z); //side

	DEBUGLOG->outdent();

	DEBUGLOG->log("FrameBufferObject Creation: GBuffer"); DEBUGLOG->indent();
	FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	FrameBufferObject fbo(shaderProgram.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default
	DEBUGLOG->outdent();

	DEBUGLOG->log("RenderPasses Creation: Trees GBuffer"); DEBUGLOG->indent();
	DEBUGLOG->log("creating " + DebugLog::to_string(treeVariants.size()) + " RenderPasses");

	std::function<void(Renderable*)> perRenderableFunc = [&] (Renderable* r)
	{
		auto e = s_renderable_material_map.find(r);
		if( e != s_renderable_material_map.end() )
		{
			auto d = s_material_texture_handles[e->second].find(aiTextureType_DIFFUSE);
			if ( d != s_material_texture_handles[e->second].end())
			{
				shaderProgram.updateAndBindTexture("tex", 5, d->second);
				shaderProgram.update("mixTexture", 1.0f);
			}else{
				shaderProgram.update("mixTexture", 0.0);
			}
			auto n = s_material_texture_handles[e->second].find(aiTextureType_NORMALS);
			if ( n != s_material_texture_handles[e->second].end())
			{
				shaderProgram.updateAndBindTexture("normalTex", 6, n->second);
				shaderProgram.update("hasNormalTex", true);
			}else{
				shaderProgram.update("hasNormalTex", false);
			}
		 }else{
			shaderProgram.update("mixTexture", 0.0);
			shaderProgram.update("hasNormalTex", false);
			shaderProgram.update("color", *s_renderable_color_map[r]);
		}
	};

	// create one render pass per tree type
	std::vector<RenderPass* > treeRenderpasses(NUM_TREE_VARIANTS);
	for ( int i = 0; i < treeVariants.size(); i++)
	{
		treeRenderpasses[i] = new RenderPass(&shaderProgram, &fbo);
		for ( auto r : treeVariants[i]->branchRenderables )
		{
			treeRenderpasses[i]->addRenderable(r);
		}
		for ( auto r : treeVariants[i]->foliageRenderables )
		{
			treeRenderpasses[i]->addRenderable(r);
		}
		treeRenderpasses[i]->setPerRenderableFunction( &perRenderableFunc );
		treeRenderpasses[i]->addEnable(GL_ALPHA_TEST); // for foliage
		treeRenderpasses[i]->addEnable(GL_DEPTH_TEST);
	}
	treeRenderpasses[0]->setClearColor(0.0,0.0,0.0,0.0);
	treeRenderpasses[0]->addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glAlphaFunc(GL_GREATER, 0);
	DEBUGLOG->outdent();

	// regular GBuffer compositing
	DEBUGLOG->log("Shader Compilation: GBuffer compositing"); DEBUGLOG->indent();
	ShaderProgram compShader("/screenSpace/fullscreen.vert", "/screenSpace/finalCompositing.frag"); DEBUGLOG->outdent();
	compShader.bindTextureOnUse("colorMap", 	 fbo.getBuffer("fragColor"));
	compShader.bindTextureOnUse("normalMap", 	 fbo.getBuffer("fragNormal"));
	compShader.bindTextureOnUse("positionMap",  fbo.getBuffer("fragPosition"));

	DEBUGLOG->log("RenderPass Creation: GBuffer Compositing"); DEBUGLOG->indent();
	Quad quad;
	RenderPass compositing(&compShader, 0);
	compositing.addClearBit(GL_COLOR_BUFFER_BIT);
	compositing.setClearColor(0.25,0.25,0.35,0.0);
	compositing.addDisable(GL_DEPTH_TEST);
	compositing.addRenderable(&quad);

	//////////////////////////////////////////////////////////////////////////////
	///////////////////////    GUI / USER INPUT   ////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	// Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, true);

	Turntable turntable;
	double old_x;
    double old_y;
	glfwGetCursorPos(window, &old_x, &old_y);
	
	auto cursorPosCB = [&](double x, double y)
	{
		ImGuiIO& io = ImGui::GetIO();
		if ( io.WantCaptureMouse )
		{ return; } // ImGUI is handling this

		double d_x = x - old_x;
		double d_y = y - old_y;

		if ( turntable.getDragActive() )
		{
			turntable.dragBy(d_x, d_y, view);
		}

		old_x = x;
		old_y = y;
	};

	auto mouseButtonCB = [&](int b, int a, int m)
	{
		if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_PRESS)
		{
			turntable.setDragActive(true);
		}
		if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_RELEASE)
		{
			turntable.setDragActive(false);
		}

		ImGui_ImplGlfwGL3_MouseButtonCallback(window, b, a, m);
	};

	 auto keyboardCB = [&](int k, int s, int a, int m)
	 {
	 	if (a == GLFW_RELEASE) {return;} 
		switch(k){
	 	case GLFW_KEY_W:
			s_eye_distance -= 0.5f;
	 		eye = glm::vec4(eye.x, eye.y, s_eye_distance, 1.0f);
	 		break;
	 	//case GLFW_KEY_A:
	 	//	eye += glm::vec4(-0.1f,0.0f,0.0f,0.0f);
	 	//	center += glm::inverse(view) * glm::vec4(-0.1f,0.0f,0.0f,0.0f);
	 	//	break;
	 	case GLFW_KEY_S:
			s_eye_distance += 0.5f;
	 		eye = glm::vec4(eye.x, eye.y, s_eye_distance, 1.0f);
	 		break;
	 	//case GLFW_KEY_D:
	 	//	eye += glm::vec4(0.1f,0.0f,0.0f,0.0f);
	 	//	center += glm::inverse(view) * glm::vec4(0.1f,0.0f,0.0f,0.0f);
	 	//	break;
	 	default:
	 		break;
		}
		ImGui_ImplGlfwGL3_KeyCallback(window,k,s,a,m);
	 };

	setCursorPosCallback(window, cursorPosCB);
	setMouseButtonCallback(window, mouseButtonCB);
	setKeyCallback(window, keyboardCB);

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////// RENDER LOOP /////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	double elapsedTime = 0.0;
	render(window, [&](double dt)
	{
		elapsedTime += dt;
		s_simulationTime = elapsedTime;
		std::string window_header = "Tree Animation Test - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered	
		ImGui::PushItemWidth(-125);
		
		ImGui::SliderFloat("windDirection", &s_wind_angle, 0.0f, 360.0f); 
		ImGui::SliderFloat("windPower", &s_wind_power, 0.0f, 4.0f); 

		bool updateAngleShifts = false;
		if (ImGui::CollapsingHeader("Angle Shifts"))
		{   ImGui::SliderFloat3("vAngleShiftFront", glm::value_ptr( angleshifts[0]), -1.0f, 1.0f);
			ImGui::SliderFloat3("vAngleShiftBack", glm::value_ptr( angleshifts[1]), -1.0f, 1.0f);
			ImGui::SliderFloat3("vAngleShiftSide", glm::value_ptr( angleshifts[2]), -1.0f, 1.0f);
			updateAngleShifts = true;
		}else {updateAngleShifts = false;}
		
		bool updateAmplitudes = false;
		if (ImGui::CollapsingHeader("Amplitudes"))
		{   ImGui::SliderFloat3("vAmplitudesFront", glm::value_ptr( amplitudes[0]), -1.0f, 1.0f);
			ImGui::SliderFloat3("vAmplitudesBack", glm::value_ptr( amplitudes[1]), -1.0f, 1.0f);
			ImGui::SliderFloat3("vAmplitudesSide", glm::value_ptr( amplitudes[2]), -1.0f, 1.0f); 
			updateAmplitudes = true;
		}else{updateAmplitudes = false;}

		bool updateFrequencies = false;
		if (ImGui::CollapsingHeader("Frequencies"))
		{   ImGui::SliderFloat3("fFrequencies", glm::value_ptr( frequencies), 0.0f, 3.0f);
			updateFrequencies = true;
		}else{ updateFrequencies =false; }
		
		ImGui::PopItemWidth();
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		view = glm::lookAt(glm::vec3(turntable.getRotationMatrix() * eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		//&&&&&&&&&&& CAMERA UNIFORMS &&&&&&&&&&&&&&//
		shaderProgram.update( "view",  view);

		//&&&&&&&&&&& FOREST UNIFORMS &&&&&&&&&&&&&&//

		//&&&&&&&&&&& SIMULATION UNIFORMS &&&&&&&&&&&&&&//
		shaderProgram.update("simTime", s_simulationTime);
		s_wind_direction = glm::rotateY(glm::vec3(0.0f,0.0f,1.0f), glm::radians(s_wind_angle));
		shaderProgram.update( "worldWindDirection", glm::vec4(s_wind_direction,s_wind_power));
		
		if (updateAngleShifts){
		shaderProgram.update("vAngleShiftFront", angleshifts[0]); //front
		shaderProgram.update("vAngleShiftBack", angleshifts[1]); //back
		shaderProgram.update("vAngleShiftSide", angleshifts[2]);} //side
		
		if (updateAmplitudes){
		shaderProgram.update("vAmplitudesFront", amplitudes[0]); //front
		shaderProgram.update("vAmplitudesBack", amplitudes[1]); //back
		shaderProgram.update("vAmplitudesSide", amplitudes[2]);} //side
		
		if (updateFrequencies){
		shaderProgram.update("fFrequencyFront", frequencies.x); //front
		shaderProgram.update("fFrequencyBack", frequencies.y); //back
		shaderProgram.update("fFrequencySide", frequencies.z);} //side

		//&&&&&&&&&&& COMPOSITING UNIFORMS &&&&&&&&&&&&&&//
		compShader.update("vLightPos", view * s_lightPos);

		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
		for(int i = 0; i < treeRenderpasses.size(); i++)
		{
			// configure shader for this tree type
			updateTreeUniforms(shaderProgram, *treeVariants[i]);
			treeRenderpasses[i]->renderInstanced(NUM_TREES_PER_VARIANT);
		}
		
		compositing.render();

		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}