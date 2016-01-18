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

static glm::vec4 s_trunk_color =  glm::vec4(107.0f / 255.0f , 68.0f / 255.0f , 35.0f /255.0f, 1.0f); // brown
static glm::vec4 s_foliage_color = glm::vec4(22.0f / 255.0f , 111.0f / 255.0f , 22.0f /255.0f, 1.0f); // green
static glm::vec4 s_lightPos = glm::vec4(2.0,2.0,2.0,1.0);

static float s_wind_angle = 45.0f;
static glm::vec3 s_wind_direction = glm::rotateY(glm::vec3(1.0f,0.0f,0.0f), glm::radians(s_wind_angle));
static glm::mat4 s_wind_rotation = glm::mat4(1.0f);
static float s_wind_power = 1.0f;

static float s_strength = 1.0f;
static bool  s_isRotating = false;

static float s_simulationTime = 0.0f;

static std::map<Renderable*, glm::vec4*> s_renderable_color_map;
static std::map<Renderable*, int> s_renderable_material_map; //!< mapping a renderable to a material index
static std::vector<std::map<aiTextureType, GLuint>> s_material_texture_handles; //!< mapping material texture types to texture handles

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

	/////////////////////    create Tree data    //////////////////////////
	DEBUGLOG->log("Setup: generating trees"); DEBUGLOG->indent();
	// generate a tree randomly
	srand (time(NULL));	
	
	float tree_height = 4.0f;
	float tree_width = tree_height / 10.0f;

	std::vector<TreeAnimation::Tree::Branch* > branches; // branches, indexed
	TreeAnimation::Tree* tree = TreeAnimation::Tree::generateTree(tree_height, tree_width, 7, 3, &branches);	
	DEBUGLOG->outdent();
	
	/////////////////////    generate Tree Renderable    //////////////////////////
	DEBUGLOG->log("Setup: generating renderables"); DEBUGLOG->indent();

	// import using ASSIMP and check for errors
	Assimp::Importer importer;
	DEBUGLOG->log("Loading branch model");
	std::string branchModel = "branch_detailed.dae";
	
	const aiScene* scene = AssimpTools::importAssetFromResourceFolder(branchModel, importer);
	std::map<aiTextureType, AssimpTools::MaterialTextureInfo> branchTexturesInfo;
	if (scene != NULL) branchTexturesInfo = AssimpTools::getMaterialTexturesInfo(scene, 0);
	if (scene != NULL) s_material_texture_handles.resize(scene->mNumMaterials);

	int i = 0;
	for (auto e : branchTexturesInfo)
	{
		GLuint texHandle = TextureTools::loadTextureFromResourceFolder(branchTexturesInfo[e.first].relativePath);
		if (texHandle != -1){ s_material_texture_handles[i][e.first] = texHandle; }
	}

	// generate one renderable per branch
	std::function<void(TreeAnimation::Tree::Branch*, std::vector<Renderable*>&)> generateRenderablesRecursively = [&](
		TreeAnimation::Tree::Branch* branch, std::vector<Renderable*>& renderables)
	{
		auto branchRenderable = TreeAnimation::generateRenderable(branch, scene); 
		if (scene != NULL) s_renderable_material_map[branchRenderable] = 0;
		s_renderable_color_map[branchRenderable] = &s_trunk_color;
		renderables.push_back( branchRenderable );

		for (int i = 0; i < branch->children.size(); i++)
		{
			generateRenderablesRecursively(branch->children[i], renderables);
			auto foliageRenderable = TreeAnimation::generateFoliage(branch->children[i], 35);
			s_renderable_color_map[foliageRenderable] = &s_foliage_color;
		
			renderables.push_back(foliageRenderable);
		}
	};
	
	std::vector<Renderable*> objects;
	generateRenderablesRecursively(branches[0], objects);
	
	TreeAnimation::WindField windField(128,128);
	windField.updateVectorTexture(0.0f);

	DEBUGLOG->outdent();
	//////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// RENDERING  ///////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	
	/////////////////////     Scene / View Settings     //////////////////////////
	glm::vec4 eye(0.0f, 0.0f, 3.0f, 1.0f);
	glm::vec4 center(0.0f,0.0f,0.0f,1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0,1,0));

	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 10.f);
	
	DEBUGLOG->log("Setup: model matrices"); DEBUGLOG->indent();
	glm::mat4 model = glm::translate(glm::vec3(0.0f, - tree_height / 2.0f, 0.0f)); 
	//glm::mat4 model = glm::mat4(1.0f);
	DEBUGLOG->outdent();

	/////////////////////// 	Renderpasses     ///////////////////////////
	 // regular GBuffer
	 DEBUGLOG->log("Shader Compilation: GBuffer"); DEBUGLOG->indent();
	 ShaderProgram shaderProgram("/treeAnim/tree.vert", "/modelSpace/GBuffer.frag"); DEBUGLOG->outdent();
	 shaderProgram.update("model", model);
	 shaderProgram.update("view", view);
	 shaderProgram.update("projection", perspective);
	 
	 //shaderProgram.printUniformInfo();
	 //shaderProgram.printInputInfo();
	 DEBUGLOG->outdent();

	 DEBUGLOG->log("FrameBufferObject Creation: GBuffer"); DEBUGLOG->indent();
	 FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	 FrameBufferObject fbo(shaderProgram.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	 FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default
	 DEBUGLOG->outdent();

	 DEBUGLOG->log("RenderPass Creation: GBuffer"); DEBUGLOG->indent();
	 RenderPass renderPass(&shaderProgram, &fbo);
	 renderPass.addEnable(GL_DEPTH_TEST);	
	 renderPass.setClearColor(0.0,0.0,0.0,0.0);
	 renderPass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	  for (auto r : objects){renderPass.addRenderable(r);}  
	 DEBUGLOG->outdent();

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
			 }
			 auto n = s_material_texture_handles[e->second].find(aiTextureType_NORMALS);
			 if ( n != s_material_texture_handles[e->second].end())
			 {
				 shaderProgram.updateAndBindTexture("normalTex", 6, n->second);
				 shaderProgram.update("hasNormalTex", true);
			 }
		 }
		 else
		 {
			shaderProgram.update("mixTexture", 0.0);
			shaderProgram.update("hasNormalTex", false);
			shaderProgram.update("color", *s_renderable_color_map[r]);
		 }
	 };
	 renderPass.setPerRenderableFunction(&perRenderableFunc);

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
	 		eye += glm::inverse(view)    * glm::vec4(0.0f,0.0f,-0.1f,0.0f);
	 		center += glm::inverse(view) * glm::vec4(0.0f,0.0f,-0.1f,0.0f);
	 		break;
	 	case GLFW_KEY_A:
	 		eye += glm::inverse(view)	 * glm::vec4(-0.1f,0.0f,0.0f,0.0f);
	 		center += glm::inverse(view) * glm::vec4(-0.1f,0.0f,0.0f,0.0f);
	 		break;
	 	case GLFW_KEY_S:
	 		eye += glm::inverse(view)    * glm::vec4(0.0f,0.0f,0.1f,0.0f);
	 		center += glm::inverse(view) * glm::vec4(0.0f,0.0f,0.1f,0.0f);
	 		break;
	 	case GLFW_KEY_D:
	 		eye += glm::inverse(view)    * glm::vec4(0.1f,0.0f,0.0f,0.0f);
	 		center += glm::inverse(view) * glm::vec4(0.1f,0.0f,0.0f,0.0f);
	 		break;
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

		static glm::vec3 angleshifts[3] ={glm::vec3(0.0),glm::vec3(0.0),glm::vec3(0.0)};
		ImGui::SliderFloat3("vAngleShiftFront", glm::value_ptr( angleshifts[0]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAngleShiftBack", glm::value_ptr( angleshifts[1]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAngleShiftSide", glm::value_ptr( angleshifts[2]), -1.0f, 1.0f);
		
		static glm::vec3 amplitudes[3] = {glm::vec3(0.3f),glm::vec3(0.3f),glm::vec3(0.3f)};
		ImGui::SliderFloat3("vAmplitudesFront", glm::value_ptr( amplitudes[0]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAmplitudesBack", glm::value_ptr( amplitudes[1]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAmplitudesSide", glm::value_ptr( amplitudes[2]), -1.0f, 1.0f); 
		
		static glm::vec3 frequencies(1.0f);
		ImGui::SliderFloat3("fFrequencies", glm::value_ptr( frequencies), 0.0f, 3.0f); 

		ImGui::PopItemWidth();

		//static float stiffness = 1.0f;
	 //   if (ImGui::CollapsingHeader("Stiffness Test"))
		//{
		//	static float length = 1.0f;
		//	static float thickness = 1.0f;
		//	static float width = 1.0f;
		//	stiffness = TreeAnimation::Tree::computeStiffness(width, thickness, length, TreeAnimation::E_APPLE);
		//	ImGui::Text("stiffness %f", stiffness);
		//	ImGui::SliderFloat("length", &length, 0.0f, 10.0f);
		//	ImGui::SliderFloat("thickness", &thickness, 0.0f, width);
		//	ImGui::SliderFloat("width", &width, 0.0f, length / 4.0f);
		//}
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update view related uniforms
		shaderProgram.update( "view",  view);
		shaderProgram.update( "model", turntable.getRotationMatrix() * model);

		shaderProgram.update("simTime", s_simulationTime);
		s_wind_direction = glm::rotateY(glm::vec3(0.0f,0.0f,1.0f), glm::radians(s_wind_angle));
		shaderProgram.update( "windDirection", s_wind_direction);
		
		glm::vec3 windTangent = glm::vec3(-s_wind_direction.z, s_wind_direction.y, s_wind_direction.x);
		float animatedWindPower = sin(s_simulationTime) * (s_wind_power / 2.0f) + s_wind_power / 2.0f + (0.25f * sin(2.0f * s_wind_power * s_simulationTime + 0.25f)) ; 
		s_wind_rotation = glm::rotate(glm::mat4(1.0f), (animatedWindPower / 2.0f), windTangent);
		shaderProgram.update( "windRotation" , s_wind_rotation); 

		shaderProgram.update("vAngleShiftFront", angleshifts[0]); //front
		shaderProgram.update("vAngleShiftBack", angleshifts[1]); //back
		shaderProgram.update("vAngleShiftSide", angleshifts[2]); //side
		
		shaderProgram.update("vAmplitudesFront", amplitudes[0]); //front
		shaderProgram.update("vAmplitudesBack", amplitudes[1]); //back
		shaderProgram.update("vAmplitudesSide", amplitudes[2]); //side
		
		shaderProgram.update("fFrequencyFront", frequencies.x); //front
		shaderProgram.update("fFrequencyBack", frequencies.y); //back
		shaderProgram.update("fFrequencySide", frequencies.z); //side

		shaderProgram.update("tree.phase", tree->m_phase); //front

		// upload tree uniforms
		for (unsigned int i = 0; i < branches.size(); i++)
		{
			std::string prefix = "tree.branches[" + DebugLog::to_string(i) + "].";

			shaderProgram.update(prefix + "origin", branches[i]->origin);
			shaderProgram.update(prefix + "phase", branches[i]->phase);	
			//shaderProgram.update(prefix + "stiffness", branches[i]->stiffness);		

			shaderProgram.update(prefix + "pseudoInertiaFactor", 1.0f);
			
			// orientation is computed from object space direction relative to optimal branch axis
			glm::quat orientation = glm::rotation(glm::vec3(0.0f,1.0f,0.0f), branches[i]->direction);
			glm::vec4 quatAsVec4 = glm::vec4(orientation.x, orientation.y, orientation.z, orientation.w);
			shaderProgram.update(prefix + "orientation", quatAsVec4);

			//int parentIdx = 0; if ( branches[i]->parent != nullptr) {parentIdx = branches[i]->parent->idx;}
			//shaderProgram.update(prefix + "parentIdx", parentIdx);
		}

		// compositing shader
		compShader.update("vLightPos", view * s_lightPos);
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
		renderPass.render();

		compositing.render();

		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}