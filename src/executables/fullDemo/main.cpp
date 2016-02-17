/*******************************************
 * **** DESCRIPTION ****
 ****************************************/
#include <iostream>
#include <time.h>

#include <Rendering/VertexArrayObjects.h>
#include <Rendering/PostProcessing.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "misc.cpp"
////////////////////// PARAMETERS /////////////////////////////
static const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);
static const glm::vec4 WORLD_LIGHT_DIRECTION = glm::vec4(-glm::normalize(glm::vec3(-2.16f, 2.6f, 10.0f)), 0.0f);

// needed for bezier-patch-interpolation
glm::mat4 bezier = glm::mat4(
	-1, 3, -3, 1,
	3, -6, 3, 0,
	-3, 3, 0, 0,
	1, 0, 0, 0
);

glm::mat4 bezier_transposed = glm::transpose(bezier);

//////////////////// MISC /////////////////////////////////////
std::map<aiTextureType, GLuint> textures;

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int main()
{

	DEBUGLOG->setAutoPrint(true);
	// create window and opengl context
	auto window = generateWindow(WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);
	
	srand(time(NULL));

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////// SETUP ////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	/////////////////////    Import Assets    ////////////////////////////////
	DEBUGLOG->log("Setup: importing assets"); DEBUGLOG->indent(); 
	Assimp::Importer importer;

	//TODO load all models that are needed

	/************ trees / branches ************/
	loadBranchModel();
	loadFoliageMaterial();
	TreeAnimation::TreeRendering treeRendering;
	generateTrees(treeRendering);
	TreeAnimation::WindField windField(64,64);
	windField.updateVectorTexture(0.0f);
	/******************************************/

	//TODO load all material information aswell ( + textures)

	/////////////////////    Import Textures    //////////////////////////////
	DEBUGLOG->outdent(); DEBUGLOG->log("Setup: importing textures"); DEBUGLOG->indent(); 
	
	// Skybox
	GLuint tex_cubeMap = TextureTools::loadDefaultCubemap();


	//TODO load all (non-material) textures that are needed
	// Tess
	GLuint distortionTex = TextureTools::loadTexture( RESOURCES_PATH "/terrain_height.png");
	GLuint diffTex = TextureTools::loadTexture( RESOURCES_PATH "/terrain_grass.jpg");
	//GLuint rockTex = TextureTools::loadTexture( RESOURCES_PATH "/terrain_rock.jpg");

	/////////////////////    Import Stuff (Misc)    //////////////////////////

	//TODO load whatever else is needed

	DEBUGLOG->outdent(); DEBUGLOG->log("Setup: Cameras / Views");DEBUGLOG->indent(); 

	Camera mainCamera; // first person camera
	mainCamera.setProjectionMatrix( glm::perspective(glm::radians(65.f), getRatio(window), 0.5f, 100.f) );

	Camera lightCamera; // used for shadow mapping
	lightCamera.setProjectionMatrix( glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.0f, 30.0f) );
	lightCamera.setPosition(- glm::vec3(WORLD_LIGHT_DIRECTION) * 15.0f);
	lightCamera.setCenter( glm::vec3( 0.0f,0.0f,0.0f) );

	// create terrain
	std::vector<Renderable* > objects;
	objects.push_back(new Terrain());

	// modelmartix for terrain
	std::vector<glm::mat4 > modelMatrices;
	modelMatrices.resize(1);
	modelMatrices[0] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,-0.5f,0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f,0.0,0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(60.0f, 7.0f, 60.0f));
	glm::mat4 model = modelMatrices[0];
	
	//////////////////////////////////////////////////////////////////////////////
	////////////////////////// RENDERING SETUP  //////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	/////////////////////// 	Renderpasses     ///////////////////////////
	DEBUGLOG->outdent(); DEBUGLOG->log("Rendering Setup: 'Geometry' Rendering"); DEBUGLOG->indent();

	// regular GBuffer
	ShaderProgram sh_gbuffer("/modelSpace/GBuffer.vert", "/modelSpace/GBuffer_mat.frag");
	sh_gbuffer.update("view",       mainCamera.getViewMatrix());
	sh_gbuffer.update("projection", mainCamera.getProjectionMatrix());
	FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	FrameBufferObject fbo_gbuffer(sh_gbuffer.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default
	RenderPass r_gbuffer(&sh_gbuffer, &fbo_gbuffer);
	r_gbuffer.addEnable(GL_DEPTH_TEST);	
	r_gbuffer.setClearColor(0.0,0.0,0.0,0.0);
	r_gbuffer.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	// Terrainstuff
	ShaderProgram shaderProgram("/tessellation/test/test_vert.vert", "/tessellation/test/test_frag_lod.frag", "/tessellation/test/test_tc_lod.tc", "/tessellation/test/test_te_bezier.te"); DEBUGLOG->outdent();//
	shaderProgram.update("model", model);
	//shaderProgram.update("view", view);
	shaderProgram.update("view", mainCamera.getViewMatrix());
	//shaderProgram.update("projection", perspective);
	shaderProgram.update("projection", mainCamera.getProjectionMatrix());
	shaderProgram.update("b", bezier);
	shaderProgram.update("bt", bezier_transposed);
	shaderProgram.bindTextureOnUse("terrain", distortionTex);
	shaderProgram.bindTextureOnUse("diff", diffTex);
	//shaderProgram.bindTextureOnUse("rock", rockTex);

	/************ trees / branches ************/
	treeRendering.createAndConfigureShaders("/modelSpace/GBuffer_mat.frag", "/treeAnim/foliage.frag");
	treeRendering.branchShader->update("projection", mainCamera.getProjectionMatrix());
	treeRendering.foliageShader->update("projection", mainCamera.getProjectionMatrix());
	treeRendering.createAndConfigureUniformBlocksAndBuffers(1);
	assignTreeMaterialTextures(treeRendering);
	assignWindFieldUniforms(treeRendering, windField);
	treeRendering.createAndConfigureRenderpasses( &fbo_gbuffer, &fbo_gbuffer );
	/******************************************/

	// TODO shadow map (light source) renderpass

	// skybox rendering (gbuffer style)
	PostProcessing::SkyboxRendering r_skybox;
	r_skybox.m_skyboxShader.update("projection", mainCamera.getProjectionMatrix());

	// TODO construct all renderpasses, shaders and framebuffer objects

	/////////////////////// 	Assign Renderables    ///////////////////////////
	DEBUGLOG->outdent(); DEBUGLOG->log("Rendering Setup: assigning renderables"); DEBUGLOG->indent();

	// TODO assign renderables to render passes that render geometry 
	
	// Terrainstuff
	RenderPass renderPass(&shaderProgram, 0);
	renderPass.addEnable(GL_DEPTH_TEST);
	renderPass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	renderPass.setClearColor(0.0, 0.0, 0.4,0.0);
	for (auto r : objects){renderPass.addRenderable(r);}

	// TODO assign all Renderables to gbuffer render pass that should be rendered by it

	/////////////////////// 	Renderpasses     ///////////////////////////
	DEBUGLOG->outdent(); DEBUGLOG->log("Rendering Setup: per-renderable functions"); DEBUGLOG->indent();

	//TODO per-renderable function that automatically updates material information and textures for r_gbuffer

	//TODO create and assign per-renderable functions to render passes (at least if it's already possible)

	/////////////////////// 	Renderpasses     ///////////////////////////
	DEBUGLOG->outdent(); DEBUGLOG->log("Rendering Setup: 'Screen-Space' Rendering"); DEBUGLOG->indent();
	Quad quad; // can be assigned to all screen-space render passes
	
	ShaderProgram sh_gbufferComp("/screenSpace/fullscreen.vert", "/screenSpace/finalCompositing_mat.frag"); DEBUGLOG->outdent();
	sh_gbufferComp.bindTextureOnUse("colorMap", 	 fbo_gbuffer.getBuffer("fragColor"));
	sh_gbufferComp.bindTextureOnUse("normalMap", 	 fbo_gbuffer.getBuffer("fragNormal"));
	sh_gbufferComp.bindTextureOnUse("positionMap",   fbo_gbuffer.getBuffer("fragPosition"));
	sh_gbufferComp.bindTextureOnUse("materialMap",   fbo_gbuffer.getBuffer("fragMaterial"));
	FrameBufferObject fbo_gbufferComp(sh_gbufferComp.getOutputInfoMap(), WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
	RenderPass r_gbufferComp(&sh_gbufferComp, &fbo_gbufferComp);
	r_gbufferComp.addDisable(GL_DEPTH_TEST);
	r_gbufferComp.addRenderable(&quad);
	DEBUGLOG->outdent();

	// Post-Processing rendering
	PostProcessing::DepthOfField r_depthOfField(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y, &quad);
	PostProcessing::LensFlare 	 r_lensFlare(fbo_gbuffer.getWidth() / 2, fbo_gbuffer.getHeight() / 2);
	//TODO Bloom //PostProcessing::BoxBlur boxBlur(fbo_gbuffer.getWidth(), fbo_gbuffer.getHeight(),&quad);

	// for arbitrary texture display
	ShaderProgram sh_showTex("/screenSpace/fullscreen.vert", "/screenSpace/simpleAlphaTexture.frag");
	RenderPass r_showTex(&sh_showTex, 0);
	r_showTex.addRenderable(&quad);
	r_showTex.addDisable(GL_DEPTH_TEST);
	r_showTex.setViewport(0,0,WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);


	//////////////////////////////////////////////////////////////////////////////
	///////////////////////    GUI / USER INPUT   ////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	
	// Setup User Input (Helper moves Cameras around)
	CallbackHelper::setupCallbackHelper(window, &mainCamera);

	CallbackHelper::cursorPosFunc = [&](double x, double y)
	{
	 	//TODO what you want to happen
	};

	CallbackHelper::mouseButtonFunc = [&](int b, int a, int m)
	{
	 	//TODO what you want to happen
	};

	 CallbackHelper::keyboardFunc = [&](int k, int s, int a, int m)
	 {
	 	//TODO what you want to happen
	 };

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////// RENDER LOOP /////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	double elapsedTime = 0.0;
	render(window, [&](double dt)
	{
		elapsedTime += dt;
		std::string window_header = "Lake Moedrianielrend - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
			
		if (ImGui::CollapsingHeader("Tree Rendering")) 
			treeRendering.imguiInterfaceSimulationProperties();

		//TODO what you want to be able to modify, use multiple windows, collapsing headers, whatever 

        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// VARIABLE UPDATING ///////////////////////////////
		mainCamera.update(dt);
		updateLightCamera(mainCamera, lightCamera);
		windField.updateVectorTexture(elapsedTime);
	
		//TODO update arbitrary variables that are dependent on time or other changes

		//////////////////////////////////////////////////////////////////////////////

		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update view dependent uniforms
		sh_gbuffer.update( "view", mainCamera.getViewMatrix());
		r_skybox.m_skyboxShader.update("view", glm::mat4(glm::mat3(mainCamera.getViewMatrix())));
		r_lensFlare.updateLensStarMatrix(mainCamera.getViewMatrix());
		sh_gbufferComp.update("vLightDir", mainCamera.getViewMatrix() * WORLD_LIGHT_DIRECTION);

		shaderProgram.update("view", mainCamera.getViewMatrix());

		treeRendering.foliageShader->update("view", mainCamera.getViewMatrix());
		treeRendering.branchShader->update("view", mainCamera.getViewMatrix());
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
		
		//TODO render whatever in whatever order necessary
		//TODO copy stuff around that has to be copied around
		//TODO funfunfun

		// render regular G-Buffer 
		r_gbuffer.render();
		
		//TODO other rendering procedures that render into G-Buffer
		
		// render trees
		for( unsigned int i = 0; i < treeRendering.branchRenderpasses.size(); i++){
			glUniformBlockBinding(treeRendering.branchShader->getShaderProgramHandle(), treeRendering.branchShaderUniformBlockInfoMap["Tree"].index, 2+i);
			treeRendering.branchRenderpasses[i]->renderInstanced(NUM_TREES_PER_VARIANT);
		}
		for(unsigned int i = 0; i < treeRendering.foliageRenderpasses.size(); i++)
		{
			glUniformBlockBinding(treeRendering.foliageShader->getShaderProgramHandle(), treeRendering.foliageShaderUniformBlockInfoMap["Tree"].index, 2+i);
			treeRendering.foliageRenderpasses[i]->renderInstanced(NUM_TREES_PER_VARIANT);
		}

		//TODO render grass
		//TODO render tesselated mountains
		renderPass.render();
		//TODO render skybox
		r_skybox.render(tex_cubeMap, &fbo_gbuffer);

		//TODO render shadow map ( most of above again )

		// render regular compositing from GBuffer
		r_gbufferComp.render();

		//TODO render water reflections 
		//TODO render god rays 

		//////////// POST-PROCESSING ////////////////////

		// Depth of Field and Lens Flare
		// r_depthOfField.execute(fbo_gbuffer.getBuffer("fragPosition"), fbo_gbufferComp.getBuffer("fragmentColor"));
		// r_lensFlare.renderLensFlare(depthOfField.m_dofCompFBO->getBuffer("fragmentColor"), 0);

		/////////// DEBUGGING ////////////////////////////
		r_showTex.setViewport(0,0, WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y );
		sh_showTex.updateAndBindTexture("tex", 0, fbo_gbufferComp.getBuffer("fragmentColor"));
		r_showTex.render();

		// show / debug view of some texture
		r_showTex.setViewport(0,0,WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
		sh_showTex.updateAndBindTexture("tex", 0, fbo_gbuffer.getBuffer("fragNormal"));
		r_showTex.render();

		r_showTex.setViewport(WINDOW_RESOLUTION.x / 4,0,WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
		sh_showTex.updateAndBindTexture("tex", 0, fbo_gbuffer.getBuffer("fragMaterial"));
		r_showTex.render();

		glViewport(0,0,WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y); // reset

		///////////// IMGUI /////////////////////////////
		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}