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
			TreeAnimation::generateFoliageGeometryShaderVertexData(b, NUM_FOLIAGE_QUADS_PER_BRANCH, fData);
			for ( auto c : b->children)
			{
				TreeAnimation::generateFoliageGeometryShaderVertexData(c, NUM_FOLIAGE_QUADS_PER_BRANCH, fData);
				TreeAnimation::generateBranchVertexData(c, bData, branchModel);
			}
		}

		if ( !fData.positions.empty())
		{
			auto fRender =TreeAnimation::generateFoliageGeometryShaderRenderable(fData);
		
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
	std::vector<TreeEntity* > treeVariants = generateTreeVariants(NUM_TREE_VARIANTS, scene);
	std::vector<glm::mat4 > treeModelMatrices = generateModelMatrices(NUM_TREES_PER_VARIANT * NUM_TREE_VARIANTS, 30.0f, 30.0f);
	TreeAnimation::SimulationProperties simulation;

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
	DEBUGLOG->log("Shader Compilation: BranchToGBuffer"); DEBUGLOG->indent();
	ShaderProgram branchShader("/treeAnim/tree.vert", "/modelSpace/GBuffer.frag"); DEBUGLOG->outdent();
	branchShader.update("view", view);
	branchShader.update("projection", perspective);
	TreeAnimation::updateSimulationUniforms(branchShader, simulation);

	DEBUGLOG->log("Shader Compilation: FoliageToGBuffer"); DEBUGLOG->indent();
	ShaderProgram foliageShader("/treeAnim/tree.vert", "/treeAnim/foliage.frag" , "/treeAnim/foliage.geom" ); DEBUGLOG->outdent();
	foliageShader.update("view", view);
	foliageShader.update("projection", perspective);
	TreeAnimation::updateSimulationUniforms(foliageShader, simulation);

	// regular GBuffer
	DEBUGLOG->log("FrameBufferObject Creation: Scene GBuffer"); DEBUGLOG->indent();
	FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	FrameBufferObject scene_gbuffer(branchShader.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	//DEBUGLOG->log("FrameBufferObject Creation: Foliage GBuffer");
	//FrameBufferObject gbuffer_foliage(foliageShader.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default
	glBindTexture(GL_TEXTURE_2D, scene_gbuffer.getBuffer("fragColor"));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (GLint) log_2(max(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y)) );
	DEBUGLOG->outdent();

	DEBUGLOG->log("RenderPasses Creation: Trees GBuffer"); DEBUGLOG->indent();
	DEBUGLOG->log("creating " + DebugLog::to_string(treeVariants.size()) + " RenderPasses");

	// assign branch textures
	if (! s_material_texture_handles.empty()){
	auto difftex = s_material_texture_handles[0].find(aiTextureType_DIFFUSE);
	if ( difftex != s_material_texture_handles[0].end())
	{
		branchShader.bindTextureOnUse("tex", difftex->second);
		branchShader.update("mixTexture", 1.0f);
	}
	auto normaltex = s_material_texture_handles[0].find(aiTextureType_NORMALS);
	if (normaltex != s_material_texture_handles[0].end())
	{
		branchShader.bindTextureOnUse("normalTex", normaltex->second);
		branchShader.update("hasNormalTex", true);
	}}

	foliageShader.bindTextureOnUse("tex", foliageTexHandle);

	// create one render pass per tree type
	std::vector<RenderPass* > branchRenderpasses(NUM_TREE_VARIANTS);
	std::vector<RenderPass* > foliageRenderpasses(NUM_TREE_VARIANTS);
	for ( int i = 0; i < treeVariants.size(); i++)
	{
		branchRenderpasses[i] = new RenderPass(&branchShader, &scene_gbuffer);
		for ( auto r : treeVariants[i]->branchRenderables )
		{
			branchRenderpasses[i]->addRenderable(r);
		}
		branchRenderpasses[i]->addEnable(GL_DEPTH_TEST);

		foliageRenderpasses[i] = new RenderPass(&foliageShader, 0);
		for ( auto r : treeVariants[i]->foliageRenderables )
		{
			foliageRenderpasses[i]->addRenderable(r);
		}
		foliageRenderpasses[i]->addEnable(GL_ALPHA_TEST); // for foliage
		foliageRenderpasses[i]->addEnable(GL_DEPTH_TEST);
	}
	branchRenderpasses[0]->setClearColor(0.0,0.0,0.0,0.0);
	branchRenderpasses[0]->addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glAlphaFunc(GL_GREATER, 0);
	DEBUGLOG->outdent();

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
		ImGui::SliderFloat("windDirection", &s_wind_angle, 0.0f, 360.0f); 
		ImGui::SliderFloat("windPower", &s_wind_power, 0.0f, 4.0f); 
		ImGui::SliderFloat("foliageSize", &s_foliage_size, 0.0f, 3.0f);	

		bool updateAngleShifts = false;
		if (ImGui::CollapsingHeader("Angle Shifts"))
		{   ImGui::SliderFloat3("vAngleShiftFront", glm::value_ptr( simulation.angleshifts[0]), -1.0f, 1.0f);
			ImGui::SliderFloat3("vAngleShiftBack", glm::value_ptr( simulation.angleshifts[1]), -1.0f, 1.0f);
			ImGui::SliderFloat3("vAngleShiftSide", glm::value_ptr( simulation.angleshifts[2]), -1.0f, 1.0f);
			updateAngleShifts = true;
		}else {updateAngleShifts = false;}
		
		bool updateAmplitudes = false;
		if (ImGui::CollapsingHeader("Amplitudes"))
		{   ImGui::SliderFloat3("vAmplitudesFront", glm::value_ptr( simulation.amplitudes[0]), -1.0f, 1.0f);
			ImGui::SliderFloat3("vAmplitudesBack", glm::value_ptr( simulation.amplitudes[1]), -1.0f, 1.0f);
			ImGui::SliderFloat3("vAmplitudesSide", glm::value_ptr( simulation.amplitudes[2]), -1.0f, 1.0f); 
			updateAmplitudes = true;
		}else{updateAmplitudes = false;}

		bool updateFrequencies = false;
		if (ImGui::CollapsingHeader("Frequencies"))
		{   ImGui::SliderFloat3("fFrequencies", glm::value_ptr( simulation.frequencies), 0.0f, 3.0f);
			updateFrequencies = true;
		}else{ updateFrequencies =false; }

		ImGui::PopItemWidth();
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		view = glm::lookAt(glm::vec3(turntable.getRotationMatrix() * eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		//&&&&&&&&&&& CAMERA UNIFORMS &&&&&&&&&&&&&&//
		branchShader.update( "view",  view);
		foliageShader.update( "view", view);

		//&&&&&&&&&&& FOREST UNIFORMS &&&&&&&&&&&&&&//

		//&&&&&&&&&&& SIMULATION UNIFORMS &&&&&&&&&&&&&&//
		branchShader.update("simTime", s_simulationTime);
		foliageShader.update("simTime", s_simulationTime);
		s_wind_direction = glm::rotateY(glm::vec3(0.0f,0.0f,1.0f), glm::radians(s_wind_angle));
		branchShader.update( "worldWindDirection", glm::vec4(s_wind_direction,s_wind_power));
		foliageShader.update("worldWindDirection", glm::vec4(s_wind_direction,s_wind_power)); //front
		foliageShader.update("foliageSize", s_foliage_size);
		
		if (updateAngleShifts){
		branchShader.update("vAngleShiftFront", simulation.angleshifts[0]); //front
		branchShader.update("vAngleShiftBack", simulation.angleshifts[1]); //back
		branchShader.update("vAngleShiftSide", simulation.angleshifts[2]);
		foliageShader.update("vAngleShiftFront", simulation.angleshifts[0]); //front
		foliageShader.update("vAngleShiftBack", simulation.angleshifts[1]); //back
		foliageShader.update("vAngleShiftSide", simulation.angleshifts[2]);} //side
		
		if (updateAmplitudes){
		branchShader.update("vAmplitudesFront", simulation.amplitudes[0]); //front
		branchShader.update("vAmplitudesBack", simulation.amplitudes[1]); //back
		branchShader.update("vAmplitudesSide", simulation.amplitudes[2]);
		foliageShader.update("vAmplitudesFront", simulation.amplitudes[0]); //front
		foliageShader.update("vAmplitudesBack", simulation.amplitudes[1]); //back
		foliageShader.update("vAmplitudesSide", simulation.amplitudes[2]);} //side
		
		if (updateFrequencies){
		branchShader.update("fFrequencyFront", simulation.frequencies.x); //front
		branchShader.update("fFrequencyBack", simulation.frequencies.y); //back
		branchShader.update("fFrequencySide", simulation.frequencies.z);
		foliageShader.update("fFrequencyFront", simulation.frequencies.x); //front
		foliageShader.update("fFrequencyBack", simulation.frequencies.y); //back
		foliageShader.update("fFrequencySide", simulation.frequencies.z);}//side
		
		//&&&&&&&&&&& COMPOSITING UNIFORMS &&&&&&&&&&&&&&//
		compShader.update("vLightPos", view * s_lightPos);
		compShader.update("strength", s_strength);
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
		// render GBuffer
		for(int i = 0; i < branchRenderpasses.size(); i++)
		{
			// configure shader for this tree type
			TreeAnimation::updateTreeUniforms(branchShader, treeVariants[i]->tree);
			branchRenderpasses[i]->renderInstanced(NUM_TREES_PER_VARIANT);
		}

		glBindTexture(GL_TEXTURE_2D,scene_gbuffer.getBuffer("fragColor")); 
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		// perform compositing
		compositing.render();

		// copy depth buffer to default fbo
		glBindFramebuffer(GL_READ_FRAMEBUFFER, scene_gbuffer.getFramebufferHandle());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0,0,WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y,0,0,WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// render foliage to screen
		for(int i = 0; i < foliageRenderpasses.size(); i++)
		{
			TreeAnimation::updateTreeUniforms(foliageShader, treeVariants[i]->tree);
			foliageRenderpasses[i]->renderInstanced(NUM_TREES_PER_VARIANT);
		}

		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}