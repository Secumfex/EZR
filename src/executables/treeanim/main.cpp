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
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(1280.0f, 720.0f);
const float TREE_HEIGHT = 4.0f;
const float TREE_WIDTH = TREE_HEIGHT / 10.0f;
const int NUM_MAIN_BRANCHES = 5;
const int NUM_SUB_BRANCHES  = 5;
const int NUM_TREE_VARIANTS = 3;
const int NUM_TREES_PER_VARIANT = 33;
const int NUM_FOLIAGE_QUADS_PER_BRANCH = 10;

static glm::vec4 s_trunk_color =  glm::vec4(107.0f / 255.0f , 68.0f / 255.0f , 35.0f /255.0f, 1.0f); // brown
static glm::vec4 s_foliage_color = glm::vec4(22.0f / 255.0f , 111.0f / 255.0f , 22.0f /255.0f, 1.0f); // green
static glm::vec4 s_lightPos = glm::vec4(0.0,50.0f,0.0,1.0);

static float s_wind_angle = 45.0f;
static glm::vec3 s_wind_direction = glm::rotateY(glm::vec3(1.0f,0.0f,0.0f), glm::radians(s_wind_angle));
static glm::mat4 s_wind_rotation = glm::mat4(1.0f);
static float s_wind_power = 0.25f;

static float s_foliage_size = 0.25f;
static bool  s_isRotating = false;

static float s_simulationTime = 0.0f;

static float s_eye_distance = 10.0f;
static float s_strength = 1.0f;

static std::map<Renderable*, glm::vec4*> s_renderable_color_map;
static std::map<Renderable*, int> s_renderable_material_map; //!< mapping a renderable to a material index
static std::vector<std::map<aiTextureType, GLuint>> s_material_texture_handles; //!< mapping material texture types to texture handles

//////////////////// MISC /////////////////////////////////////
float randFloat(float min, float max) //!< returns a random number between min and max
{
	return (((float) rand() / (float) RAND_MAX) * (max - min) + min); 
}
double log_2( double n )  
{  
    return log( n ) / log( 2 );      // log(n)/log(2) is log_2. 
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

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
	glBindTexture(GL_TEXTURE_2D, foliageTexHandle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	/////////////////////    create Tree data           //////////////////////////
	DEBUGLOG->log("Setup: generating trees"); DEBUGLOG->indent();
	srand (time(NULL));	

	// generate a forest randomly, including renderables
	TreeAnimation::TreeRendering treeRendering;
	treeRendering.generateAndConfigureTreeEntities(
		NUM_TREE_VARIANTS,
		TREE_HEIGHT, TREE_WIDTH,
		NUM_MAIN_BRANCHES, NUM_SUB_BRANCHES,
		NUM_FOLIAGE_QUADS_PER_BRANCH,
		scene);

	treeRendering.generateModelMatrices(
		NUM_TREES_PER_VARIANT,
		-15.0f, 15.0f, -15.0f, 15.0f);

	treeRendering.createInstanceMatrixAttributes();
	DEBUGLOG->outdent();

	/////////////////////    create wind field          //////////////////////////
	DEBUGLOG->indent();
	TreeAnimation::WindField windField(64,64);
	windField.updateVectorTexture(0.0f);
	glm::vec4 windFieldArea(-15.0f, -15.f, 15.0f, 15.0f);
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
	DEBUGLOG->log("Shader Compilation: BranchToGBuffer & FoliageToGBuffer"); DEBUGLOG->indent();
	
	treeRendering.createAndConfigureShaders("/modelSpace/GBuffer.frag", "/treeAnim/foliage.frag");

	treeRendering.branchShader->update("view", view);
	treeRendering.branchShader->update("projection", perspective);
	treeRendering.foliageShader->update("view", view);
	treeRendering.foliageShader->update("projection", perspective);
	
	treeRendering.createAndConfigureUniformBlocksAndBuffers(1);
	DEBUGLOG->outdent();

	// regular GBuffer
	DEBUGLOG->log("FrameBufferObject Creation: Scene GBuffer"); DEBUGLOG->indent();
	FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	FrameBufferObject scene_gbuffer(treeRendering.branchShader->getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	//DEBUGLOG->log("FrameBufferObject Creation: Foliage GBuffer");
	//FrameBufferObject gbuffer_foliage(foliageShader.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default
	glBindTexture(GL_TEXTURE_2D, scene_gbuffer.getBuffer("fragColor"));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (GLint) log_2(max(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y)) );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	DEBUGLOG->outdent();

	DEBUGLOG->log("RenderPasses Creation: Trees GBuffer"); DEBUGLOG->indent();
	DEBUGLOG->log("creating " + DebugLog::to_string(treeRendering.treeEntities.size()) + " RenderPasses");

	// assign branch textures
	if (! s_material_texture_handles.empty()){
	auto difftex = s_material_texture_handles[0].find(aiTextureType_DIFFUSE);
	if ( difftex != s_material_texture_handles[0].end())
	{
		treeRendering.branchShader->bindTextureOnUse("tex", difftex->second);
		treeRendering.branchShader->update("mixTexture", 1.0f);
	}
	auto normaltex = s_material_texture_handles[0].find(aiTextureType_NORMALS);
	if (normaltex != s_material_texture_handles[0].end())
	{
		treeRendering.branchShader->bindTextureOnUse("normalTex", normaltex->second);
		treeRendering.branchShader->update("hasNormalTex", true);
	}}

	treeRendering.foliageShader->bindTextureOnUse("tex", foliageTexHandle);

	// windfield
	treeRendering.branchShader->bindTextureOnUse( "windField", windField.m_vectorTextureHandle);
	treeRendering.foliageShader->bindTextureOnUse("windField", windField.m_vectorTextureHandle);
	treeRendering.branchShader->update( "windFieldArea", windFieldArea);
	treeRendering.foliageShader->update("windFieldArea", windFieldArea);
	
	// create one render pass per tree type
	treeRendering.createAndConfigureRenderpasses( &scene_gbuffer, 0 );
	
	// also set clear bits for first render pass
	treeRendering.branchRenderpasses[0]->setClearColor(0.0,0.0,0.0,0.0);
	treeRendering.branchRenderpasses[0]->addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	DEBUGLOG->outdent();

	
	Grid grid(30,30, 1.0f, 1.0f,true);
	ShaderProgram gridShader("/modelSpace/GBuffer.vert", "/modelSpace/GBuffer.frag");
	RenderPass gridRenderPass(&gridShader, 0);
	gridRenderPass.addRenderable(&grid);
	gridRenderPass.addEnable(GL_DEPTH_TEST);
	gridRenderPass.addDisable(GL_BLEND);
	gridShader.bindTextureOnUse("tex", windField.m_vectorTextureHandle);
	gridShader.update("mixTexture", 1.0f);
	gridShader.update("model", glm::rotate(glm::radians(90.0f), glm::vec3(1.0f,0.0f,0.0f)));
	gridShader.update("projection", perspective);
	gridShader.update("view", view);

	// regular GBuffer compositing
	DEBUGLOG->log("Shader Compilation: GBuffer compositing"); DEBUGLOG->indent();
	ShaderProgram compShader("/screenSpace/fullscreen.vert", "/screenSpace/finalCompositing.frag"); DEBUGLOG->outdent();
	compShader.bindTextureOnUse("colorMap", 	 scene_gbuffer.getBuffer("fragColor"));
	compShader.bindTextureOnUse("normalMap", 	 scene_gbuffer.getBuffer("fragNormal"));
	compShader.bindTextureOnUse("positionMap",   scene_gbuffer.getBuffer("fragPosition"));
	compShader.update("strength", s_strength);

	DEBUGLOG->log("RenderPass Creation: GBuffer Compositing"); DEBUGLOG->indent();
	Quad quad;
	RenderPass compositing(&compShader, 0);
	compositing.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	compositing.setClearColor(0.25,0.25,0.35,0.0);
	compositing.addDisable(GL_DEPTH_TEST);
	compositing.addRenderable(&quad);

	DEBUGLOG->log("Shader Compilation: Bloom Post Process"); DEBUGLOG->indent();
	ShaderProgram bloomShader("/screenSpace/fullscreen.vert", "/screenSpace/postProcessBloomMipMap.frag" ); DEBUGLOG->outdent();
	bloomShader.update("power", 2.0);
	bloomShader.update("depth", s_strength);
	bloomShader.bindTextureOnUse("tex", scene_gbuffer.getBuffer("fragColor"));

	DEBUGLOG->log("RenderPass Creation: Bloom Post Process"); DEBUGLOG->indent();
	RenderPass bloom(&bloomShader, 0);
	bloom.addEnable(GL_BLEND);
	bloom.addDisable(GL_DEPTH_TEST);
	bloom.addRenderable(&quad);

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

		ImGui::SliderFloat("strength", &s_strength, 0.0f, 4.0f); 
		ImGui::SliderFloat("windPower", &s_wind_power, 0.0f, 4.0f); 
		ImGui::SliderFloat("foliageSize", &s_foliage_size, 0.0f, 3.0f);	

		static bool showWindField = false;
		ImGui::Checkbox("Show Wind Field", &showWindField);
		
		treeRendering.imguiInterfaceSimulationProperties();

		ImGui::PopItemWidth();
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		view = glm::lookAt(glm::vec3(turntable.getRotationMatrix() * eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		windField.updateVectorTexture(s_simulationTime);
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		//&&&&&&&&&&& CAMERA UNIFORMS &&&&&&&&&&&&&&//
		treeRendering.branchShader->update( "view",  view);
		treeRendering.foliageShader->update( "view", view);
		gridShader.update("view", view);

		//&&&&&&&&&&& FOREST UNIFORMS &&&&&&&&&&&&&&//

		//&&&&&&&&&&& SIMULATION UNIFORMS &&&&&&&&&&&&&&//
		treeRendering.branchShader->update("simTime", s_simulationTime);
		treeRendering.foliageShader->update("simTime", s_simulationTime);
		s_wind_direction = glm::rotateY(glm::vec3(0.0f,0.0f,1.0f), glm::radians(s_wind_angle));
		treeRendering.branchShader->update( "windPower", s_wind_power);
		treeRendering.foliageShader->update("windPower", s_wind_power); //front

		treeRendering.foliageShader->update("foliageSize", s_foliage_size);
		
		treeRendering.updateActiveImguiInterfaces();

		//&&&&&&&&&&& COMPOSITING UNIFORMS &&&&&&&&&&&&&&//
		compShader.update("vLightPos", view * s_lightPos);
		bloomShader.update("depth", s_strength);
		bloomShader.update("intensity", 1.0 / s_strength);
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
		// render GBuffer
		for(int i = 0; i < treeRendering.branchRenderpasses.size(); i++)
		{
			// configure shader for this tree type
			glUniformBlockBinding(treeRendering.branchShader->getShaderProgramHandle(), treeRendering.branchShaderUniformBlockInfoMap["Tree"].index, 2+i);
			treeRendering.branchRenderpasses[i]->renderInstanced(NUM_TREES_PER_VARIANT);
		}

		// perform compositing
		compositing.render();

		// copy depth buffer to default fbo
		glBindFramebuffer(GL_READ_FRAMEBUFFER, scene_gbuffer.getFramebufferHandle());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0,0,WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y,0,0,WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// render foliage to screen
		for(int i = 0; i < treeRendering.foliageRenderpasses.size(); i++)
		{
			// change uniform block binding point
			glUniformBlockBinding(treeRendering.foliageShader->getShaderProgramHandle(), treeRendering.foliageShaderUniformBlockInfoMap["Tree"].index, 2+i);
			treeRendering.foliageRenderpasses[i]->renderInstanced(NUM_TREES_PER_VARIANT);
		}

		// copy composited image from screen to scene color texture
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glReadBuffer(GL_BACK);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scene_gbuffer.getFramebufferHandle());
		glBlitFramebuffer(0,0,WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y,0,0,WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, scene_gbuffer.getBuffer("fragColor"));
		glGenerateMipmap(GL_TEXTURE_2D);
		glBlendFunc(GL_ONE, GL_ONE); // this is altered by ImGui::Render(), so reset it every frame
		bloom.render();

		if ( showWindField) gridRenderPass.render();

		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}