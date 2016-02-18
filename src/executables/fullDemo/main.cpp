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
#include <VML/VolumetricLighting.h>

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

static float s_wind_power = 0.25f;
static float s_foliage_size = 0.4f;

static float s_grass_size = 0.2f;

static int s_ssrRayStep = 0.0f;

//////////////////// MISC /////////////////////////////////////
std::map<aiTextureType, GLuint> textures;

static bool s_show_debug_views = true;

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
	//GLuint distortionTex = TextureTools::loadTexture( RESOURCES_PATH "/terrain_height2.png");
	GLuint distortionTex = TextureTools::loadTexture( RESOURCES_PATH "/heightmap.jpg");
	GLuint terrainNormalTex = TextureTools::loadTexture( RESOURCES_PATH "/terrain_normal.png");

	GLuint diffTex = TextureTools::loadTexture( RESOURCES_PATH "/Rocks_Seamless_1_COLOR.png");
	glBindTexture(GL_TEXTURE_2D, diffTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GLuint snowTex = TextureTools::loadTexture( RESOURCES_PATH "/terrain_snow.jpg");
	glBindTexture(GL_TEXTURE_2D, snowTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	GLuint tex_grassQuad = TextureTools::loadTextureFromResourceFolder("grass.png");


	/////////////////////    Import Stuff (Misc)    //////////////////////////

	//TODO load whatever else is needed

	DEBUGLOG->outdent(); DEBUGLOG->log("Setup: Cameras / Views");DEBUGLOG->indent(); 

	Camera mainCamera; // first person camera
	mainCamera.setProjectionMatrix( glm::perspective(glm::radians(65.f), getRatio(window), 0.5f, 100.f) );

	Camera lightCamera; // used for shadow mapping
	lightCamera.setProjectionMatrix( glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -15.0f, 30.0f) );
	lightCamera.setPosition(- glm::vec3(WORLD_LIGHT_DIRECTION) * 15.0f);
	lightCamera.setCenter( glm::vec3( 0.0f,0.0f,0.0f) );

	// create terrain
	std::vector<Renderable* > objects;
	objects.push_back(new Terrain());

	// modelmartix for terrain
	glm::vec4 terrainRange(-75.0f, -75.f, 75.0f, 75.0f);
	std::vector<glm::mat4 > modelMatrices;
	modelMatrices.resize(1);
	// glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f,0.0,0.0)) *
	//modelMatrices[0] = glm::translate(glm::mat4(1.0f), glm::vec3(-50.0f, -1.5f, -50.0f)) *  glm::scale(glm::mat4(1.0), glm::vec3(130.0f, 15.0f, 130.0f));
	modelMatrices[0] = glm::translate(glm::mat4(1.0f), glm::vec3(-75.0f, 0.0f, -75.0f)) *  glm::scale(glm::mat4(1.0), glm::vec3(150.0f, 17.0f, 150.0f));
	glm::mat4 model = modelMatrices[0];
	
	// grid resembling water surface
	Renderable* waterGrid = new Grid(32,32,1.0f,1.0f,true);
	glm::mat4 modelWater = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f,0.0,0.0));
	// grid resembling grass spawning area
	Renderable* grassGrid = new Grid(64,64,0.5f,0.5f,true);
	glm::mat4 modelGrass = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f,0.0,0.0));

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
	ShaderProgram sh_tessellation("/tessellation/test/test_vert.vert", "/tessellation/test/test_frag_lod.frag", "/tessellation/test/test_tc_lod.tc", "/tessellation/test/test_te.te"); DEBUGLOG->outdent();//
	sh_tessellation.update("model", model);
	sh_tessellation.update("view", mainCamera.getViewMatrix());
	sh_tessellation.update("projection", mainCamera.getProjectionMatrix());
	sh_tessellation.update("b", bezier);
	sh_tessellation.update("bt", bezier_transposed);
	sh_tessellation.bindTextureOnUse("terrain", distortionTex);
	sh_tessellation.bindTextureOnUse("diff", diffTex);
	sh_tessellation.bindTextureOnUse("snow", snowTex);
	sh_tessellation.bindTextureOnUse("normal", terrainNormalTex);


	RenderPass r_terrain(&sh_tessellation, &fbo_gbuffer);
	r_terrain.addEnable(GL_DEPTH_TEST);
	//r_terrain.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	r_terrain.setClearColor(0.0, 0.0, 0.0,0.0);
	for (auto r : objects){r_terrain.addRenderable(r);}

	// setup variables for shadowmapping
	FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	FrameBufferObject shadowMap(1024, 1024);
	FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default

	// setup shaderprogram
	ShaderProgram shadowMapShader("/vml/shadowmap.vert", "/vml/shadowmap.frag");
	RenderPass shadowMapRenderpass(&shadowMapShader, &shadowMap);
	shadowMapShader.update("projection", lightCamera.getProjectionMatrix());
	
	// setup renderpass
	shadowMapRenderpass.addClearBit(GL_DEPTH_BUFFER_BIT);
	shadowMapRenderpass.addEnable(GL_DEPTH_TEST);
	
	/************ trees / branches ************/
	treeRendering.createAndConfigureShaders("/modelSpace/GBuffer_mat.frag", "/treeAnim/foliage.frag");
	treeRendering.branchShader->update("projection", mainCamera.getProjectionMatrix());
	treeRendering.foliageShader->update("projection", mainCamera.getProjectionMatrix());
	treeRendering.branchShadowMapShader->update("projection", lightCamera.getProjectionMatrix());
	treeRendering.foliageShadowMapShader->update("projection", lightCamera.getProjectionMatrix());
	treeRendering.createAndConfigureUniformBlocksAndBuffers(1);
	assignTreeMaterialTextures(treeRendering);
	assignWindFieldUniforms(treeRendering, windField);
	assignHeightMapUniforms(treeRendering, distortionTex, terrainRange);
	treeRendering.createAndConfigureRenderpasses( &fbo_gbuffer, &fbo_gbuffer, &shadowMap );
	/******************************************/

	// skybox rendering (gbuffer style)
	PostProcessing::SkyboxRendering r_skybox;
	r_skybox.m_skyboxShader.update("projection", mainCamera.getProjectionMatrix());

	ShaderProgram sh_grassGeom("/modelSpace/geometry.vert", "/modelSpace/GBuffer_mat.frag", "/geometry/simpleGeom.geom");
	RenderPass r_grassGeom(&sh_grassGeom, &fbo_gbuffer);
	// geom.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	r_grassGeom.addRenderable(grassGrid);
	r_grassGeom.addEnable(GL_DEPTH_TEST);
	r_grassGeom.addEnable(GL_ALPHA_TEST);
	//r_grassGeom.addEnable(GL_BLEND);
	sh_grassGeom.update("projection", mainCamera.getProjectionMatrix());
	sh_grassGeom.bindTextureOnUse("tex", tex_grassQuad);
	sh_grassGeom.bindTextureOnUse("vectorTexture", windField.m_vectorTextureHandle);
	sh_grassGeom.update("model", modelGrass);
	sh_grassGeom.update("mixTexture", 1.0f);
	sh_grassGeom.update("materialType", 0.0f);
	sh_grassGeom.update("shininess", 3.0f);
	sh_grassGeom.update("shininess_strength", 0.1f);
	sh_grassGeom.update("strength", s_grass_size); // grass size
	sh_grassGeom.bindTextureOnUse("heightMap", distortionTex);
	sh_grassGeom.update("heightMapRange", terrainRange); // grass size
	glAlphaFunc(GL_GREATER, 0);

	// TODO construct all renderpasses, shaders and framebuffer objects

	/////////////////////// 	Assign Renderables    ///////////////////////////
	DEBUGLOG->outdent(); DEBUGLOG->log("Rendering Setup: assigning renderables"); DEBUGLOG->indent();

	// TODO assign renderables to render passes that render geometry 
	


	// assign all Renderables to gbuffer render pass that should be rendered by it
	r_gbuffer.addRenderable(waterGrid);

	// assign all Renderables to shadow map render pass that should and can be rendered by it
	shadowMapRenderpass.addRenderable(waterGrid);

	/////////////////////// 	Renderpasses     ///////////////////////////
	DEBUGLOG->outdent(); DEBUGLOG->log("Rendering Setup: per-renderable functions"); DEBUGLOG->indent();

	//TODO per-renderable function that automatically updates material information and textures for r_gbuffer
	std::function<void(Renderable*)> r_gbuffer_perRenderableFunc = [&](Renderable* r)
	{
		if (r == waterGrid)
		{
			sh_gbuffer.update("materialType", 2.0f);
			sh_gbuffer.update("model", modelWater);
			sh_gbuffer.update("color", glm::vec4(0.2f,0.2f,0.7f,1.0f));
		}
	};
	r_gbuffer.setPerRenderableFunction(&r_gbuffer_perRenderableFunc);

	std::function<void(Renderable*)> r_shadowMap_perRenderableFunc = [&](Renderable* r)
	{
		if (r == waterGrid)
		{
			shadowMapShader.update("model", modelWater);
		}
	};
	shadowMapRenderpass.setPerRenderableFunction(&r_shadowMap_perRenderableFunc);

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

	//ssr render pass
	ShaderProgram sh_ssr("/screenSpaceReflection/screenSpaceReflection.vert", "/screenSpaceReflection/screenSpaceReflection2.frag"); DEBUGLOG->outdent();
	sh_ssr.update("screenWidth",getResolution(window).x);
	sh_ssr.update("screenHeight",getResolution(window).y);
	sh_ssr.update("camNearPlane", 0.5f);
	sh_ssr.update("camFarPlane", 100.0f);
	sh_ssr.update("user_pixelStepSize",s_ssrRayStep);
	sh_ssr.update("projection",mainCamera.getProjectionMatrix());
	sh_ssr.bindTextureOnUse("vsPositionTex",fbo_gbuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT2));
	sh_ssr.bindTextureOnUse("vsNormalTex",fbo_gbuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT1));
	sh_ssr.bindTextureOnUse("ReflectanceTex",fbo_gbuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT4));
	sh_ssr.bindTextureOnUse("DepthTex",fbo_gbuffer.getDepthTextureHandle());
	sh_ssr.bindTextureOnUse("DiffuseTex",fbo_gbufferComp.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));	//aus beleuchtung
	//sh_ssr.bindTextureOnUse("DiffuseTex",gFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
	FrameBufferObject fbo_ssr(sh_ssr.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	RenderPass r_ssr(&sh_ssr, &fbo_ssr);
	r_ssr.setClearColor(0.0,0.0,0.0,0.0);
	r_ssr.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	// Post-Processing rendering
	PostProcessing::DepthOfField r_depthOfField(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y, &quad);
	PostProcessing::LensFlare 	 r_lensFlare(fbo_gbuffer.getWidth() / 2, fbo_gbuffer.getHeight() / 2);
	//TODO Bloom //PostProcessing::BoxBlur boxBlur(fbo_gbuffer.getWidth(), fbo_gbuffer.getHeight(),&quad);

	// volume light rendering
	VolumetricLighting r_volumetricLighting(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
	r_volumetricLighting.setupNoiseTexture();
	r_volumetricLighting._raymarchingShader->bindTextureOnUse("shadowMap", shadowMap.getDepthTextureHandle());
	r_volumetricLighting._raymarchingShader->bindTextureOnUse("worldPosMap", fbo_gbuffer.getBuffer("fragPosition"));

	// for arbitrary texture display
	ShaderProgram sh_showTex("/screenSpace/fullscreen.vert", "/screenSpace/simpleAlphaTexture.frag");
	RenderPass r_showTex(&sh_showTex, 0);
	r_showTex.addRenderable(&quad);
	r_showTex.addDisable(GL_DEPTH_TEST);
	r_showTex.setViewport(0,0,WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);

	// arbitrary texture display shader
	float weightMin = 0.0f;
	float weightMax = 0.5f;
	int mode = 0;
	ShaderProgram sh_addTexShader("/screenSpace/fullscreen.vert", "/screenSpace/postProcessVolumetricLighting.frag");
	sh_addTexShader.update("min", weightMin);
	sh_addTexShader.update("max", weightMax);
	sh_addTexShader.update("mode", mode);
	RenderPass r_addTex(&sh_addTexShader, &fbo_gbufferComp);
	r_addTex.addRenderable(&quad);
	r_addTex.addDisable(GL_DEPTH_TEST);
	r_addTex.addDisable(GL_BLEND);
	sh_addTexShader.bindTextureOnUse("tex", fbo_gbufferComp.getBuffer("fragmentColor"));
	sh_addTexShader.bindTextureOnUse("addTex", r_volumetricLighting._raymarchingFBO->getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));	
	sh_addTexShader.update("strength", 0.5f);		

	//ssr stuff
	r_ssr.addRenderable(&quad);


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
		 if ( k == GLFW_KEY_P && a == GLFW_PRESS)
		 {
			 s_show_debug_views = !s_show_debug_views;
		 }
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
		
		// Tree Rendering
		bool active_interface_treeRendering = ImGui::CollapsingHeader("Tree Rendering");
		if (active_interface_treeRendering) 
		{
			treeRendering.imguiInterfaceSimulationProperties();

			ImGui::SliderFloat("foliage size", &s_foliage_size, 0.0f, 3.0f);
			ImGui::SliderFloat("wind power", &s_wind_power, 0.0f, 3.0f); 
		}
		
		// Volumetric Light
		if (ImGui::CollapsingHeader("Volumetric Lighting"))
			r_volumetricLighting.imguiInterfaceSimulationProperties();

		if (ImGui::CollapsingHeader("VML Composition"))
		{
			ImGui::SliderFloat("min", &weightMin, 0.0f, 1.0f);
			ImGui::SliderFloat("max", &weightMax, 0.0f, 1.0f);
			//std::string values[5] = {"cos", "inverse", "sqrt", "quad", "ln"};
			//const char* valuesChar = values->c_str();
			//enum MODES {COS, INVERSE, SQRT, QUAD, LN} postProcessMode;
			ImGui::Combo("mode", &mode, "cos\0sin\0inverse\0sqrt\0quad\0ln\0");
			std::cout << "mode is " << mode << std::endl;
		}
		
		// Grass
		if ( ImGui::CollapsingHeader("Grass"))
		{	
			ImGui::SliderFloat("grass size", &s_grass_size, 0.0f, 1.5f);
			sh_grassGeom.update("strength", s_grass_size);
		}
		
		// Post-Processing
		if ( ImGui::TreeNode("Post-Processing"))
		{	
			if (ImGui::TreeNode("Lens-Flare"))
			{
				r_lensFlare.imguiInterfaceEditParameters();
				r_lensFlare.updateUniforms();
			 	ImGui::TreePop();
			}
			if (ImGui::TreeNode("Depth-Of-Field")){
				r_depthOfField.imguiInterfaceEditParameters();
				r_depthOfField.updateUniforms();
			 	ImGui::TreePop();
			}
		 	ImGui::TreePop();
		}

		//TODO what you want to be able to modify, use multiple windows, collapsing headers, whatever
		
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// VARIABLE UPDATING ///////////////////////////////
		mainCamera.update(dt);
		updateLightCamera(mainCamera, lightCamera, - glm::vec3(WORLD_LIGHT_DIRECTION) * 15.0f);
		windField.updateVectorTexture(elapsedTime);
		glm::mat4 cameraView = mainCamera.getViewMatrix();
		glm::vec3 cameraPos = mainCamera.getPosition();
		glm::mat4 lightView = lightCamera.getViewMatrix();
		glm::mat4 lightProjection = lightCamera.getProjectionMatrix();
		r_volumetricLighting.update(cameraView, cameraPos, lightView, lightProjection);
	
		//TODO update arbitrary variables that are dependent on time or other changes

		//////////////////////////////////////////////////////////////////////////////

		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update view dependent uniforms
		sh_gbuffer.update( "view", mainCamera.getViewMatrix());
		r_skybox.m_skyboxShader.update("view", glm::mat4(glm::mat3(mainCamera.getViewMatrix())));
		sh_grassGeom.update("view", mainCamera.getViewMatrix());

		r_lensFlare.updateLensStarMatrix(mainCamera.getViewMatrix());
		sh_gbufferComp.update("vLightDir", mainCamera.getViewMatrix() * WORLD_LIGHT_DIRECTION);
		treeRendering.foliageShader->update("vLightDir", mainCamera.getViewMatrix() * WORLD_LIGHT_DIRECTION);

		sh_tessellation.update("view", mainCamera.getViewMatrix());
		
		shadowMapShader.update("view", lightCamera.getViewMatrix());

//		sh_grassGeom.update("model", glm::translate(glm::mat4(1.0f), mainCamera.getPosition()) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f,0.0,0.0) ));

		treeRendering.foliageShader->update("view", mainCamera.getViewMatrix());
		treeRendering.branchShader->update("view", mainCamera.getViewMatrix());
		treeRendering.branchShadowMapShader->update("view", lightCamera.getViewMatrix());
		treeRendering.foliageShadowMapShader->update("view", lightCamera.getViewMatrix());

		// wind related uniforms
		treeRendering.branchShader->update( "windPower", s_wind_power);
		treeRendering.foliageShader->update("windPower", s_wind_power);
		treeRendering.branchShadowMapShader->update("windPower", s_wind_power);
		treeRendering.foliageShadowMapShader->update("windPower", s_wind_power);

		treeRendering.foliageShader->update("foliageSize", s_foliage_size);
		treeRendering.foliageShadowMapShader->update("foliageSize", s_foliage_size);
		
		treeRendering.updateActiveImguiInterfaces();

		// vml composition
		sh_addTexShader.update("min", weightMin);
		sh_addTexShader.update("max", weightMax);
		sh_addTexShader.update("mode", mode);

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

		//TODO render tesselated mountains
		r_terrain.render();
		//render skybox
		r_skybox.render(tex_cubeMap, &fbo_gbuffer);

		//TODO render shadow map ( most of above again )
		shadowMapRenderpass.render();
		for(unsigned int i = 0; i < treeRendering.foliageShadowMapRenderpasses.size(); i++)
		{
			glUniformBlockBinding(treeRendering.foliageShadowMapShader->getShaderProgramHandle(), treeRendering.foliageShadowMapShaderUniformBlockInfoMap["Tree"].index, 2+i);
			treeRendering.foliageShadowMapRenderpasses[i]->renderInstanced(NUM_TREES_PER_VARIANT);
		}
		for(unsigned int i = 0; i < treeRendering.branchShadowMapRenderpasses.size(); i++)
		{
			glUniformBlockBinding(treeRendering.branchShadowMapShader->getShaderProgramHandle(), treeRendering.branchShadowMapShaderUniformBlockInfoMap["Tree"].index, 2+i);
			treeRendering.branchShadowMapRenderpasses[i]->renderInstanced(NUM_TREES_PER_VARIANT);
		}

		// render grass
		r_grassGeom.render();

		// render regular compositing from GBuffer
		r_gbufferComp.render();

		//TODO render water reflections 
	//	r_ssr.render();
		//TODO render god rays
		//r_volumetricLighting._raymarchingRenderPass->render();

		// overlay volumetric lighting
		//r_addTex.render();

		//////////// POST-PROCESSING ////////////////////

		// Depth of Field and Lens Flare
		//r_depthOfField.execute(fbo_gbuffer.getBuffer("fragPosition"), fbo_gbufferComp.getBuffer("fragmentColor"));
		//r_lensFlare.renderLensFlare(r_depthOfField.m_dofCompFBO->getBuffer("fragmentColor"), &fbo_gbufferComp);

		// quick debug
		//copyFBOContent(r_depthOfField.m_dofCompFBO, &fbo_gbufferComp, GL_COLOR_BUFFER_BIT); 

		/////////// DEBUGGING ////////////////////////////
		r_showTex.setViewport(0,0, WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y );
		sh_showTex.updateAndBindTexture("tex", 0, fbo_gbufferComp.getBuffer("fragmentColor"));
		r_showTex.render();
		
		if (s_show_debug_views)
		{

		// show / debug view of some texture
		r_showTex.setViewport(0,0,WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
		sh_showTex.updateAndBindTexture("tex", 0, fbo_gbuffer.getBuffer("fragNormal"));
		r_showTex.render();

		r_showTex.setViewport(WINDOW_RESOLUTION.x / 4,0,WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
		sh_showTex.updateAndBindTexture("tex", 0, fbo_gbuffer.getBuffer("fragMaterial"));
		r_showTex.render();

		// show shadowmap
		r_showTex.setViewport(WINDOW_RESOLUTION.x / 2,0,WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
		sh_showTex.updateAndBindTexture("tex", 0, shadowMap.getDepthTextureHandle());
		r_showTex.render();

		// raymarching
		r_showTex.setViewport(3 * WINDOW_RESOLUTION.x / 4,0,WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
		sh_showTex.updateAndBindTexture("tex", 0, r_volumetricLighting._raymarchingFBO->getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
		r_showTex.render();

		glViewport(0,0,WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y); // reset
		}

		///////////// IMGUI /////////////////////////////
		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}
