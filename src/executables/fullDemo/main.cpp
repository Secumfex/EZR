/*******************************************
 * **** DESCRIPTION ****
 ****************************************/
#include <iostream>
#include <time.h>

#include <Rendering/VertexArrayObjects.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "misc.cpp"
#include <VML/VolumetricLighting.h>

#include <windows.h>
#include <mmsystem.h>

////////////////////// PARAMETERS /////////////////////////////
static glm::vec2 WINDOW_RESOLUTION = glm::vec2(1024.0f, 576.0f);
static const glm::vec4 WORLD_LIGHT_DIRECTION = glm::vec4(-glm::normalize(glm::vec3(-2.16f, 2.6f, 10.0f)), 0.0f);

// needed for bezier-patch-interpolation
glm::mat4 bezier = glm::mat4(
	-1, 3, -3, 1,
	3, -6, 3, 0,
	-3, 3, 0, 0,
	1, 0, 0, 0
);

glm::mat4 bezier_transposed = glm::transpose(bezier);

std::unordered_map<aiTextureType, GLuint, AssimpTools::EnumClassHash> textures;
//////////////////// MISC /////////////////////////////////////

static Timer s_idle_ui_timer(true);
static Timer s_idle_movement_timer(true);
static const double IDLE_ANIMATION_TIME_LIMIT = 20.0;
static bool s_idle_animation_active = false;
static glm::mat4 s_idle_animation_rotation_matrix;

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
	GLuint distortionTex = TextureTools::loadTexture( RESOURCES_PATH "/heightmap2.jpg");
	GLuint terrainNormalTex = TextureTools::loadTexture( RESOURCES_PATH "/terrain_normal.png");

	GLuint diffTex = TextureTools::loadTexture( RESOURCES_PATH "/Rocks_Seamless_1_COLOR.png");
	OPENGLCONTEXT->bindTexture(diffTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GLuint snowTex = TextureTools::loadTexture( RESOURCES_PATH "/terrain_snow.jpg");
	OPENGLCONTEXT->bindTexture(snowTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GLuint grassTex = TextureTools::loadTexture( RESOURCES_PATH "/terrain_grass.jpg");
	OPENGLCONTEXT->bindTexture(grassTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	GLuint tex_grassQuad = TextureTools::loadTextureFromResourceFolder("grass_2.png");
	OPENGLCONTEXT->bindTexture(tex_grassQuad);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 2);

	GLuint waterTextureHandle = TextureTools::loadTextureFromResourceFolder("water/07_DIFFUSE.jpg");
	OPENGLCONTEXT->bindTexture(waterTextureHandle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GLuint waterNormalTextureHandle = TextureTools::loadTextureFromResourceFolder("water/07_NORMAL.jpg");
	OPENGLCONTEXT->bindTexture(waterNormalTextureHandle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


	/////////////////////    Import Stuff (Misc)    //////////////////////////

	//TODO load whatever else is needed

	DEBUGLOG->outdent(); DEBUGLOG->log("Setup: Cameras / Views");DEBUGLOG->indent(); 

	Camera mainCamera; // first person camera
	mainCamera.setPosition(0.0f, 2.0f, 0.0f);
	mainCamera.setDirection(glm::vec3(0.0f, 0.2f, 1.0f));
	mainCamera.setProjectionMatrix( glm::perspective(glm::radians(65.f), getRatio(window), 0.5f, 100.f) );

	Camera lightCamera; // used for shadow mapping
	lightCamera.setProjectionMatrix( glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -90.0f, 40.0f) );
	lightCamera.setPosition(- glm::vec3(WORLD_LIGHT_DIRECTION) * 15.0f);
	lightCamera.setCenter( glm::vec3( 0.0f,0.0f,0.0f) );

	// create terrain
	std::vector<Renderable* > objects;
	objects.push_back(new Terrain());

	// modelmartix for terrain
	glm::vec4 terrainRange(-100.0f, -115.f, 100.0f, 115.0f);
	std::vector<glm::mat4 > modelMatrices;
	modelMatrices.resize(1);
	// glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f,0.0,0.0)) *
	//modelMatrices[0] = glm::translate(glm::mat4(1.0f), glm::vec3(-50.0f, -1.5f, -50.0f)) *  glm::scale(glm::mat4(1.0), glm::vec3(130.0f, 15.0f, 130.0f));
	modelMatrices[0] = glm::translate(glm::mat4(1.0f), glm::vec3(-100.0f, -1.9f, -115.0f)) *  glm::scale(glm::mat4(1.0), glm::vec3(200.0f, 50.0f, 230.0f));
	glm::mat4 modelTerrain = modelMatrices[0];
	
	// grid resembling water surface
	Renderable* waterGrid = new Grid(32,32,2.0f,2.0f,true);
	glm::mat4 modelWater = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f,0.0,0.0));
	// grid resembling grass spawning area
	Renderable* grassGrid = new Grid(300,300,0.3f,0.3f,true);
	glm::mat4 modelGrass = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f,0.0,0.0));

	// sound loop
	PlaySound(TEXT( RESOURCES_PATH "/forest.wav"), NULL, SND_FILENAME | SND_LOOP | SND_ASYNC);

	//////////////////////////////////////////////////////////////////////////////
	////////////////////////// RENDERING SETUP  //////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	/////////////////////// 	Renderpasses     ///////////////////////////
	DEBUGLOG->outdent(); DEBUGLOG->log("Rendering Setup: 'Geometry' Rendering"); DEBUGLOG->indent();

	// regular GBuffer
	ShaderProgram sh_gbuffer("/modelSpace/GBuffer.vert", "/modelSpace/GBuffer_mat.frag"); // vs. not working: waterGBuffer_mat.frag
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
	sh_tessellation.update("model", modelTerrain);
	sh_tessellation.update("view", mainCamera.getViewMatrix());
	sh_tessellation.update("projection", mainCamera.getProjectionMatrix());
	sh_tessellation.update("heightZones", glm::vec4(0.1f, 0.15f, 0.4f, 0.5f)); // grassEnd stoneBegin stoneEnd SnowBegin 
	//sh_tessellation.update("b", bezier);
	//sh_tessellation.update("bt", bezier_transposed);
	sh_tessellation.bindTextureOnUse("terrain", distortionTex);
	sh_tessellation.bindTextureOnUse("diff", diffTex);
	sh_tessellation.bindTextureOnUse("snow", snowTex);
	sh_tessellation.bindTextureOnUse("normal", terrainNormalTex);
	sh_tessellation.bindTextureOnUse("grass", grassTex);


	RenderPass r_terrain(&sh_tessellation, &fbo_gbuffer);
	r_terrain.addEnable(GL_DEPTH_TEST);
	//r_terrain.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	r_terrain.setClearColor(0.0, 0.0, 0.0,0.0);
	for (auto r : objects){r_terrain.addRenderable(r);}

	ShaderProgram sh_terrainShadowmap("/tessellation/test/test_vert.vert", "/vml/shadowmap.frag", "/tessellation/test/test_tc_lod.tc", "/tessellation/test/test_te.te"); DEBUGLOG->outdent();	
	sh_terrainShadowmap.update("model", modelTerrain);
	sh_terrainShadowmap.update("view", lightCamera.getViewMatrix());
	sh_terrainShadowmap.update("projection", lightCamera.getProjectionMatrix());
	//sh_tessellation.update("b", bezier);
	//sh_tessellation.update("bt", bezier_transposed);
	sh_terrainShadowmap.bindTextureOnUse("terrain", distortionTex);
	//sh_terrainShadowmap.bindTextureOnUse("diff", diffTex);
	//sh_terrainShadowmap.bindTextureOnUse("snow", snowTex);
	//sh_terrainShadowmap.bindTextureOnUse("normal", terrainNormalTex);
	


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

	// setup renderpass for terrain shadowmap
	RenderPass r_terrainShadowMap(&sh_terrainShadowmap, &shadowMap);
	r_terrainShadowMap.addEnable(GL_DEPTH_TEST);
	//r_terrainShadowMap.setClearColor(0.0, 0.0, 0.0,0.0);
	for (auto r : objects){r_terrainShadowMap.addRenderable(r);}

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
	sh_grassGeom.update("strength",Settings.grass_size); // grass size
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
			sh_gbuffer.update("shininess", 15.0f);
			sh_gbuffer.update("shininess_strength", 0.8f);
			sh_gbuffer.update("model", modelWater);
			//sh_gbuffer.update("color", glm::vec4(0.2f,0.2f,0.7f,1.0f));
			sh_gbuffer.update("mixTexture", 1.0f);
			sh_gbuffer.updateAndBindTexture("tex", 1,waterTextureHandle);
			sh_gbuffer.update("hasNormalTex",Settings.waterHasNormalTex);
			sh_gbuffer.updateAndBindTexture("normalTex", 2, terrainNormalTex);
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
	//sh_ssr.update("user_pixelStepSize",Settings.ssrRayStep);
	sh_ssr.update("projection",mainCamera.getProjectionMatrix());
	sh_ssr.update("view",mainCamera.getViewMatrix());
	sh_ssr.update("toggleCM",Settings.ssrCubeMap);
	sh_ssr.update("toggleFade",Settings.ssrFade);
	sh_ssr.update("toggleGlossy",Settings.ssrGlossy);
	sh_ssr.update("loops",Settings.ssrLoops);
	sh_ssr.update("mixV",Settings.ssrMix);
	sh_ssr.bindTextureOnUse("vsPositionTex",fbo_gbuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT2));
	sh_ssr.bindTextureOnUse("vsNormalTex",fbo_gbuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT1));
	sh_ssr.bindTextureOnUse("ReflectanceTex",fbo_gbuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT4));
	sh_ssr.bindTextureOnUse("DepthTex",fbo_gbuffer.getDepthTextureHandle());
	sh_ssr.bindTextureOnUse("DiffuseTex",fbo_gbufferComp.getBuffer("fragmentColor"));	//aus beleuchtung

	sh_ssr.bindTextureOnUse("CubeMapTex",tex_cubeMap);
	//sh_ssr.bindTextureOnUse("DiffuseTex",gFBO.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
	FrameBufferObject fbo_ssr(sh_ssr.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	RenderPass r_ssr(&sh_ssr, &fbo_ssr);
	// r_ssr.addClearBit(GL_COLOR_BUFFER_BIT);

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

	ShaderProgram sh_addTexShader("/screenSpace/fullscreen.vert", "/screenSpace/postProcessVolumetricLighting.frag");
	sh_addTexShader.update("min", Settings.weightMin);
	sh_addTexShader.update("max", Settings.weightMax);
	sh_addTexShader.update("mode", Settings.mode);
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

	 	if ( CallbackHelper::active_mouse_control )
	 	{
	 		s_idle_movement_timer.reset();
	 		s_idle_animation_active = false;
	 	}
	};

	CallbackHelper::mouseButtonFunc = [&](int b, int a, int m)
	{
	 	//TODO what you want to happen

		ImGuiIO& io = ImGui::GetIO();
	 	if (io.WantCaptureMouse && a == GLFW_PRESS && b == GLFW_MOUSE_BUTTON_LEFT)
	 	{
	 		s_idle_ui_timer.reset();
	 	}

	};

	 CallbackHelper::keyboardFunc = [&](int k, int s, int a, int m)
	 {
		 if ( k == GLFW_KEY_P && a == GLFW_PRESS)
		 {
			Settings.show_debug_views = !Settings.show_debug_views;
		 }

		 if ( k == GLFW_KEY_O && a == GLFW_PRESS)
		 {
			Settings.dynamicDoF = !Settings.dynamicDoF;
		 }

		 if ( k == GLFW_KEY_1 && a == GLFW_PRESS)
		 {
			Settings.enableLandscape = !Settings.enableLandscape;
		 }

		 if ( k == GLFW_KEY_2 && a == GLFW_PRESS)
		 {
			Settings.enableGrass = !Settings.enableGrass;
		 }

		 if ( k == GLFW_KEY_3 && a == GLFW_PRESS)
		 {
			Settings.enableTrees = !Settings.enableTrees;
		 }

		 if ( k == GLFW_KEY_4 && a == GLFW_PRESS)
		 {
			Settings.enableSSR = !Settings.enableSSR;
		 }

		 if ( k == GLFW_KEY_5 && a == GLFW_PRESS)
		 {
			Settings.enableVolumetricLighting = !Settings.enableVolumetricLighting;
		 }

		 if ( k == GLFW_KEY_6 && a == GLFW_PRESS)
		 {
			Settings.enableDepthOfField = !Settings.enableDepthOfField;
		 }

		 if ( k == GLFW_KEY_7 && a == GLFW_PRESS)
		 {
			Settings.enableLenseflare = !Settings.enableLenseflare;
		 }

		 if ( k == GLFW_KEY_T && a == GLFW_PRESS)
		 {
			Settings.waterHasNormalTex = !Settings.waterHasNormalTex;
		 }

		 if ( k == GLFW_KEY_9 && a == GLFW_PRESS)
		 {
			Settings.enableLandscape = true;
			Settings.enableGrass = true;
			Settings.enableTrees = true;
			Settings.enableSSR = true;
			Settings.enableVolumetricLighting = true;
			Settings.enableDepthOfField = true;
			Settings.enableLenseflare = true;
		 }

		 if ( k == GLFW_KEY_0 && a == GLFW_PRESS)
		 {
			Settings.enableLandscape = false;
			Settings.enableGrass = false;
			Settings.enableTrees = false;
			Settings.enableSSR = false;
			Settings.enableVolumetricLighting = false;
			Settings.enableDepthOfField = false;
			Settings.enableLenseflare = false;
		 }

		 if ( k == GLFW_KEY_I && a == GLFW_PRESS )
		 {
			Settings.animate_seasons = !Settings.animate_seasons;
		 }
 		 if ( k == GLFW_KEY_R && a == GLFW_PRESS)
		 {
			resetSettings();

			// reset VML
			r_volumetricLighting._lightColor = Settings.lightcolor;
			r_volumetricLighting._radiocity = Settings.radiocity;
			r_volumetricLighting._useAnisotropicScattering = Settings.useALS;
		    r_volumetricLighting._raymarchingShader->update("lightColor", Settings.lightcolor);
		   	r_volumetricLighting._raymarchingShader->update("phi", 		  Settings.radiocity);
		   	r_volumetricLighting._raymarchingShader->update("useALS", 	  Settings.useALS);

		   	// reset Lens Flare
	   		r_lensFlare.m_scale  = Settings.vml_scale;
			r_lensFlare.m_bias = Settings.vml_bias;
			r_lensFlare.m_num_ghosts = Settings.vml_num_ghosts;
			r_lensFlare.m_blur_strength = Settings.vml_blur_strength;
			r_lensFlare.m_ghost_dispersal = Settings.vml_ghost_dispersal;
			r_lensFlare.m_halo_width = Settings.vml_halo_width;
			r_lensFlare.m_distortion = Settings.vml_distortion;
			r_lensFlare.m_strength = Settings.vml_strength;
			r_lensFlare.updateUniforms();

			//reset DoF
			r_depthOfField.m_focusPlaneDepths  = glm::vec4(2.0,4.0,7.0,10.0);
			// r_depthOfField.m_focusPlaneRadi = glm::vec2(10.0f, -5.0f);
			// r_depthOfField.m_farRadiusRescale = 2.0f;
			r_depthOfField.m_disable_near_field = false;
			r_depthOfField.m_disable_far_field = false;
			r_depthOfField.updateUniforms();

			ImGui::SetNextWindowPos(ImVec2(50,50));
			ImGui::SetNextWindowSize(ImVec2(500,200));
			ImGui::SetNextWindowCollapsed(false);
		 }
 		//  if ( k == GLFW_KEY_M && a == GLFW_PRESS)
		 // {
			// Settings.multithreaded_windfield = !Settings.multithreaded_windfield;
			//  DEBUGLOG->log("multithread: ",Settings.multithreaded_windfield);
		 // }
		if (k == GLFW_KEY_W || k == GLFW_KEY_S || k == GLFW_KEY_A || k == GLFW_KEY_D)
		{
			s_idle_movement_timer.reset();
			s_idle_animation_active = false;
		}
	 };

	auto windowResizeCB = [&](int width, int height)
	{
		OPENGLCONTEXT->updateWindowCache();
		WINDOW_RESOLUTION = glm::vec2(OPENGLCONTEXT->cacheWindowSize);

		mainCamera.setProjectionMatrix(glm::perspective(glm::radians(65.f), getRatio(window), 0.5f, 100.f));
		
		// update all projection matrices
		sh_ssr.update("projection",mainCamera.getProjectionMatrix());
		sh_grassGeom.update("projection", mainCamera.getProjectionMatrix());
		r_skybox.m_skyboxShader.update("projection", mainCamera.getProjectionMatrix());
		treeRendering.branchShader->update("projection", mainCamera.getProjectionMatrix());
		treeRendering.foliageShader->update("projection", mainCamera.getProjectionMatrix());
		sh_gbuffer.update("projection", mainCamera.getProjectionMatrix());
		sh_tessellation.update("projection", mainCamera.getProjectionMatrix());
	};

	setWindowResizeCallback(window, windowResizeCB);

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////// RENDER LOOP /////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	double elapsedTime = 0.0;

	render(window, [&](double dt)
	{
		// update timers
		s_idle_movement_timer.update(dt);
		s_idle_ui_timer.update(dt);

		if ( s_idle_ui_timer.getElapsedTime() > IDLE_ANIMATION_TIME_LIMIT && s_idle_movement_timer.getElapsedTime() > IDLE_ANIMATION_TIME_LIMIT)
		{
			s_idle_animation_active = true;
		}

		timings.updateReadyTimings();
		
		elapsedTime += dt;
		std::string window_header = "Lake Moedrianielrend - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
		//timings.resetTimer("gui");
		//timings.beginTimer("gui");
		
		ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered

		// Landscape
		if (ImGui::TreeNode("Tesselation"))
		{
			ImGui::Checkbox("enable", &Settings.enableLandscape);
			ImGui::TreePop();
		}

		// Tree Rendering
		if (ImGui::TreeNode("Tree Rendering"))
		{
			ImGui::Checkbox("enable", &Settings.enableTrees);
			// treeRendering.imguiInterfaceSimulationProperties();

			ImGui::SliderFloat("foliage size", &Settings.foliage_size, 0.0f, 3.0f);
			ImGui::SliderFloat("wind power", &Settings.wind_power, 0.0f, 3.0f);
			ImGui::TreePop();
		}
		
		// Volumetric Light
		if (ImGui::TreeNode("Volumetric Lighting"))
		{
			ImGui::Checkbox("enable", &Settings.enableVolumetricLighting);
			if (ImGui::TreeNode("Simulation"))
			{
				r_volumetricLighting.imguiInterfaceSimulationProperties();
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Composition"))
			{
				ImGui::SliderFloat("min", &Settings.weightMin, 0.0f, 1.0f);
				ImGui::SliderFloat("max", &Settings.weightMax, 0.0f, 1.0f);
				ImGui::Combo("mode", &Settings.mode, "const\0cos\0sin\0inverse\0sqrt\0quad\0ln\0x^4\0");
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
		
		// Grass
		if ( ImGui::TreeNode("Grass"))
		{
			ImGui::Checkbox("enable", &Settings.enableGrass);
			ImGui::SliderFloat("grass size", &Settings.grass_size, 0.0f, 1.5f);
			ImGui::TreePop();
		}

		//SSR
		if ( ImGui::TreeNode("SSR"))
		{
			ImGui::Checkbox("enable", &Settings.enableSSR);
			ImGui::SliderInt("Loops",&Settings.ssrLoops, 25, 250);
			//ImGui::SliderInt("PixelStepSize",&Settings.ssrRayStep,0,20);
			ImGui::Checkbox("toggle CubeMap", &Settings.ssrCubeMap);
			ImGui::SliderFloat("mix water",&Settings.ssrMix, 0.0, 1.0);
			ImGui::Checkbox("fade to edges", &Settings.ssrFade);
			ImGui::Checkbox("toggle glossy", &Settings.ssrGlossy);
			ImGui::Checkbox("toggle normalmap", &Settings.waterHasNormalTex);
			ImGui::TreePop();
		}

		// Post-Processing
		if ( ImGui::TreeNode("Post-Processing"))
		{	
			if (ImGui::TreeNode("Lens-Flare"))
			{
				ImGui::Checkbox("enable", &Settings.enableLenseflare);
				r_lensFlare.imguiInterfaceEditParameters();
				r_lensFlare.updateUniforms();
			 	ImGui::TreePop();
			}
			if (ImGui::TreeNode("Depth-Of-Field")){
				ImGui::Checkbox("enable", &Settings.enableDepthOfField);
				r_depthOfField.imguiInterfaceEditParameters();
				r_depthOfField.updateUniforms();
				imguiDynamicFieldOfView(r_depthOfField);
			 	ImGui::TreePop();
			}
		 	ImGui::TreePop();
		}

		if ( ImGui::TreeNode("Timings"))
		{
			timings.setEnabled(true);
			timings.imguiTimings();
			ImGui::TreePop();
		}
		else {timings.setEnabled(false);}

		if (ImGui::Button("Reset Camera")) {
			mainCamera.setPosition(0.0f, 2.0f, 0.0f);
			mainCamera.setDirection(glm::vec3(0.0f, 0.2f, 1.0f));
			updateLightCamera(mainCamera, lightCamera, - glm::vec3(WORLD_LIGHT_DIRECTION) * 15.0f);
		}
		if (ImGui::Button("Toggle Debug Views")) {
			Settings.show_debug_views = !Settings.show_debug_views;
		}
		if (ImGui::Button("Toggle Seasons Animation")) {
			Settings.animate_seasons = !Settings.animate_seasons;
		}
		//TODO what you want to be able to modify, use multiple windows, collapsing headers, whatever
		//timings.stopTimer("gui");
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// VARIABLE UPDATING ///////////////////////////////
		timings.resetTimer("varupdates");
		timings.beginTimer("varupdates");

		if (s_idle_animation_active) // update rotation matrix
		{
			s_idle_animation_rotation_matrix = glm::rotate(glm::mat4(1.0f), (float) elapsedTime / 10.0f, glm::vec3(0.0f, 1.0f, 0.0f) );

			mainCamera.setPosition(glm::vec3( s_idle_animation_rotation_matrix * glm::vec4(7.0f,1.25f,0.0f,1.0f) ) + glm::vec3(0.f,0.0f,5.0f));
			mainCamera.setCenter(glm::vec3(0.0f,1.75f,5.0f));
		}

		mainCamera.update(dt);
		updateLightCamera(mainCamera, lightCamera, - glm::vec3(WORLD_LIGHT_DIRECTION) * 15.0f);
		
		// if( Settings.multithreaded_windfield )
		// {
		// 	windField.updateVectorTextureThreaded(elapsedTime);
		// }
		// else
		// {
			windField.updateVectorTexture(elapsedTime);
		// }

		glm::mat4 cameraView = mainCamera.getViewMatrix();
		glm::vec3 cameraPos = mainCamera.getPosition();
		glm::mat4 lightView = lightCamera.getViewMatrix();
		glm::mat4 lightProjection = lightCamera.getProjectionMatrix();
		r_volumetricLighting.update(cameraView, cameraPos, lightView, lightProjection);
		
		if ( Settings.animate_seasons )
		{
			animateSeasons(treeRendering, sh_grassGeom, elapsedTime / 2.0f, Settings.grass_size, Settings.wind_power, Settings.foliage_size, sh_tessellation);
		}
		
		timings.stopTimer("varupdates");
		//////////////////////////////////////////////////////////////////////////////

		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		timings.resetTimer("uniformupdates");
		timings.beginTimer("uniformupdates");

		// update view dependent uniforms
		sh_gbuffer.update( "view", mainCamera.getViewMatrix());
		r_skybox.m_skyboxShader.update("view", glm::mat4(glm::mat3(mainCamera.getViewMatrix())));
		sh_grassGeom.update("view", mainCamera.getViewMatrix());

		r_lensFlare.updateLensStarMatrix(mainCamera.getViewMatrix());
		sh_gbufferComp.update("vLightDir", mainCamera.getViewMatrix() * WORLD_LIGHT_DIRECTION);
		treeRendering.foliageShader->update("vLightDir", mainCamera.getViewMatrix() * WORLD_LIGHT_DIRECTION);

		sh_ssr.update("view",mainCamera.getViewMatrix());
		
		shadowMapShader.update("view", lightCamera.getViewMatrix());

		sh_tessellation.update("view", mainCamera.getViewMatrix());
		sh_terrainShadowmap.update("view", lightCamera.getViewMatrix());

		treeRendering.foliageShader->update("view", mainCamera.getViewMatrix());
		treeRendering.branchShader->update("view", mainCamera.getViewMatrix());
		treeRendering.branchShadowMapShader->update("view", lightCamera.getViewMatrix());
		treeRendering.foliageShadowMapShader->update("view", lightCamera.getViewMatrix());

		// wind related uniforms
		treeRendering.branchShader->update( "windPower", Settings.wind_power);
		treeRendering.foliageShader->update("windPower", Settings.wind_power);
		treeRendering.branchShadowMapShader->update("windPower", Settings.wind_power);
		treeRendering.foliageShadowMapShader->update("windPower", Settings.wind_power);

		treeRendering.foliageShader->update("foliageSize", Settings.foliage_size);
		treeRendering.foliageShadowMapShader->update("foliageSize", Settings.foliage_size);
		
		treeRendering.updateActiveImguiInterfaces();

		// vml composition
		sh_addTexShader.update("min", Settings.weightMin);
		sh_addTexShader.update("max", Settings.weightMax);
		sh_addTexShader.update("mode", Settings.mode);
		
		sh_grassGeom.update("strength", Settings.grass_size);

		updateDynamicFieldOfView(r_depthOfField, fbo_gbuffer, dt);

		sh_ssr.update("toggleCM",Settings.ssrCubeMap);
		sh_ssr.update("toggleFade",Settings.ssrFade);
		sh_ssr.update("toggleGlossy",Settings.ssrGlossy);
		sh_ssr.update("loops",Settings.ssrLoops);
		//sh_ssr.update("user_pixelStepSize",Settings.ssrRayStep);
		sh_ssr.update("mixV",Settings.ssrMix);

		sh_gbuffer.update("time", elapsedTime);

		//std::cout<<"ZEIT: "<< elapsedTime << endl;
		timings.stopTimer("uniformupdates");
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
		
		//TODO render whatever in whatever order necessary
		//TODO copy stuff around that has to be copied around
		//TODO funfunfun

		// render regular G-Buffer 
		timings.resetTimer("gbuffer");
		timings.beginTimer("gbuffer");
		r_gbuffer.render();
		timings.stopTimer("gbuffer");
		//TODO other rendering procedures that render into G-Buffer
		
		// render trees
		timings.resetTimer("trees");
		if (Settings.enableTrees)
		{
			timings.beginTimer("trees");
			for( unsigned int i = 0; i < treeRendering.branchRenderpasses.size(); i++){
				glUniformBlockBinding(treeRendering.branchShader->getShaderProgramHandle(), treeRendering.branchShaderUniformBlockInfoMap["Tree"].index, 2+i);
				treeRendering.branchRenderpasses[i]->renderInstanced(NUM_TREES_PER_VARIANT);
			}
			for(unsigned int i = 0; i < treeRendering.foliageRenderpasses.size(); i++)
			{
				glUniformBlockBinding(treeRendering.foliageShader->getShaderProgramHandle(), treeRendering.foliageShaderUniformBlockInfoMap["Tree"].index, 2+i);
				treeRendering.foliageRenderpasses[i]->renderInstanced(NUM_TREES_PER_VARIANT);
			}
			timings.stopTimer("trees");
		}


		//TODO render tesselated mountains
		
		timings.resetTimer("landscape");
		if (Settings.enableLandscape)
		{
			timings.beginTimer("landscape");
			r_terrain.render();
			timings.stopTimer("landscape");
		}

		//render skybox
		timings.resetTimer("skybox");
		timings.beginTimer("skybox");
		r_skybox.render(tex_cubeMap, &fbo_gbuffer);
		timings.stopTimer("skybox");

		// render shadow map ( most of above again )
		timings.resetTimer("shadowmap");
		timings.beginTimer("shadowmap");
		shadowMapRenderpass.render();
		if (Settings.enableLandscape)
		{
			r_terrainShadowMap.render();
		}
		timings.stopTimer("shadowmap");

		timings.resetTimer("treesShadow");
		if (Settings.enableTrees)
		{
		timings.beginTimer("treesShadow");
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
		timings.stopTimer("treesShadow");
		}
		// render grass
		timings.resetTimer("grass");
		if (Settings.enableGrass) {
			timings.beginTimer("grass");
			r_grassGeom.render();
			timings.stopTimer("grass");
		}

		// render regular compositing from GBuffer
		timings.resetTimer("compositing");
		timings.beginTimer("compositing");
		r_gbufferComp.render();
		timings.stopTimer("compositing");

		// ssr
		timings.resetTimer("ssr");
		if (Settings.enableSSR) {
			timings.beginTimer("ssr");
			r_ssr.render();
			copyFBOContent(&fbo_ssr, &fbo_gbufferComp, GL_COLOR_BUFFER_BIT);
			timings.stopTimer("ssr");
		}
		
		// volumetric lighting
		timings.resetTimer("vml");
		if (Settings.enableVolumetricLighting) {
			timings.beginTimer("vml");
			r_volumetricLighting._raymarchingRenderPass->render();

			// overlay volumetric lighting
			r_addTex.render();
			timings.stopTimer("vml");
		}
		

		//////////// POST-PROCESSING ////////////////////

		// Depth of Field and Lens Flare
		timings.resetTimer("dof");
		if (Settings.enableDepthOfField)
		{
			timings.beginTimer("dof");

			r_depthOfField.execute(fbo_gbuffer.getBuffer("fragPosition"), fbo_gbufferComp.getBuffer("fragmentColor"));
			copyFBOContent(r_depthOfField.m_dofCompFBO, &fbo_gbufferComp, GL_COLOR_BUFFER_BIT);

			timings.stopTimer("dof");
		}

		timings.resetTimer("lensflare");
		if(Settings.enableLenseflare)
		{
			timings.beginTimer("lensflare");
			r_lensFlare.renderLensFlare( fbo_gbufferComp.getBuffer("fragmentColor"), &fbo_gbufferComp );
			timings.stopTimer("lensflare");
		}

		/////////// DEBUGGING ////////////////////////////
		r_showTex.setViewport(0,0, WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y );
		sh_showTex.updateAndBindTexture("tex", 0, fbo_gbufferComp.getBuffer("fragmentColor"));
		r_showTex.render();
		
		if (Settings.show_debug_views)
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
		if (Settings.enableVolumetricLighting)
		{
			r_showTex.setViewport(3 * WINDOW_RESOLUTION.x / 4,0,WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
			sh_showTex.updateAndBindTexture("tex", 0, r_volumetricLighting._raymarchingFBO->getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
			r_showTex.render();
		}


		OPENGLCONTEXT->setViewport(0,0,WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y); // reset
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
