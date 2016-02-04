/*******************************************
 * **** DESCRIPTION ****
 ****************************************/

#include <iostream>
#include <time.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <Importing/AssimpTools.h>

#include <Rendering/GLTools.h>
#include <Rendering/VertexArrayObjects.h>
#include <Rendering/RenderPass.h>
#include <Rendering/PostProcessing.h>

 #include "UI/imgui/imgui.h"
 #include <UI/imguiTools.h>
#include <UI/Turntable.h>

 #include <Importing/TextureTools.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

////////////////////// PARAMETERS /////////////////////////////
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);

static glm::vec4 s_color = glm::vec4(0.45 * 0.3f, 0.44f * 0.3f, 0.87f * 0.3f, 1.0f); // far : blueish
static glm::vec4 s_light_position = glm::vec4(-2.16f, 2.6f, 10.0f,1.0);
static glm::vec3 s_scale = glm::vec3(1.0f,1.0f,1.0f);
static float s_lensflare_scale = 5.0f;
static float s_lensflare_bias = -0.9f;
static int s_lensflare_num_ghosts = 3;
static float s_lensflare_ghost_dispersal = 0.6f;
static float s_lensflare_halo_width = 0.44f;
static float s_lensflare_strength = 0.7f;

static glm::vec4 s_focusPlaneDepths = glm::vec4(2.0,4.0,7.0,10.0);
static glm::vec2 s_focusPlaneRadi = glm::vec2(10.0f, -5.0f);
static float s_farRadiusRescale = 2.0f;

static float s_strength = 1.0f;
//////////////////// MISC /////////////////////////////////////
float randFloat(float min, float max) //!< returns a random number between min and max
{
	return (((float) rand() / (float) RAND_MAX) * (max - min) + min); 
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int main()
{
	DEBUGLOG->setAutoPrint(true);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////// INIT //////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	/////////////////////    Import Assets (Host memory)    //////////////////////////
	// create window and opengl context
	auto window = generateWindow(WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);

	DEBUGLOG->log("Setup: importing assets"); DEBUGLOG->indent();
	// import using ASSIMP and check for errors
	Assimp::Importer importer;
	const aiScene* scene = AssimpTools::importAssetFromResourceFolder("cube.dae", importer);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// RENDERING  ///////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	
	/////////////////////     Scene / View Settings     //////////////////////////
	glm::vec4 eye(0.0f, 0.0f, 5.0f, 1.0f);
	glm::vec4 center(0.0f,0.0f,0.0f,1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0,1,0));

	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 30.f);

	//objects
	std::vector<Renderable* > objects;

	//positions
	DEBUGLOG->log("Setup: model matrices"); DEBUGLOG->indent();
	std::vector<glm::mat4> modelMatrices(6);
	modelMatrices[0] = glm::translate(glm::vec3(randFloat(-3.0f,-2.0f), 0.0f, randFloat(-3,-2.0f)));
	modelMatrices[1] = glm::translate(glm::vec3(randFloat(0.0f,1.0f),randFloat(2.0f,3.0f),randFloat(-5.0f,5.0f)));
	modelMatrices[2] = glm::translate(glm::vec3(randFloat(-1.0f,1.0f),randFloat(-1.0f,0.0f),randFloat(0.0f,5.0f)));
	modelMatrices[3] = glm::translate(glm::vec3(randFloat(0.5f,1.0f), randFloat(-5.0,-1.0),randFloat(0.0,5.0f)));
	modelMatrices[4] = glm::translate(glm::vec3(randFloat(1.0f,5.0f), randFloat(0.0,2.0),randFloat(0.0,5.0f)));
	modelMatrices[5] = glm::translate(glm::vec3(0.0f,-2.0f,0.0f)) * glm::rotate(glm::radians(90.0f), glm::vec3(1.0f,0.0,0.0));
	DEBUGLOG->outdent();

	srand(time(NULL));
	std::vector<glm::vec4> colors(6);
	colors[0] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[1] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[2] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[3] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[4] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[5] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);

	/////////////////////     Upload assets (create Renderables / VAOs from data)    //////////////////////////
	DEBUGLOG->log("Setup: creating VAOs from mesh data"); DEBUGLOG->indent();
//	std::vector<AssimpTools::RenderableInfo> renderableInfoVector = AssimpTools::createSimpleRenderablesFromScene( scene );
	auto vertexData = AssimpTools::createVertexDataInstancesFromScene(scene);
	auto renderables = AssimpTools::createSimpleRenderablesFromVertexDataInstances(vertexData);
	for (int i = 0; i < 5; i++)
	{
		objects.push_back(renderables[0]);
	}
	objects.push_back(new Grid(10,10,1.0,1.0,true));
	
	// upload textures used by mesh
	std::map<aiTextureType, GLuint> textures;
	for (int i = 0; i < scene->mNumMaterials; i++)
	{
		auto matInfo = AssimpTools::getMaterialInfo(scene, i);
		DEBUGLOG->indent();
			AssimpTools::printMaterialInfo(matInfo);
		DEBUGLOG->outdent();
		for (auto t : matInfo.texture) // load all textures used with this material
		{
			GLuint tex = TextureTools::loadTextureFromResourceFolder(t.second.relativePath);
			if (tex != -1){ textures[t.first] = tex; } // save if successfull
		}
	}

	DEBUGLOG->outdent();

	/////////////////////// 	Renderpasses     ///////////////////////////
	// regular GBuffer
	DEBUGLOG->log("Shader Compilation: GBuffer"); DEBUGLOG->indent();
	ShaderProgram shaderProgram("/modelSpace/GBuffer.vert", "/modelSpace/GBuffer.frag"); DEBUGLOG->outdent();
	shaderProgram.update("view", view);
	shaderProgram.update("projection", perspective);

	// check for displayable textures 
	if (textures.find(aiTextureType_DIFFUSE) != textures.end())
	{ shaderProgram.bindTextureOnUse("tex", textures.at(aiTextureType_DIFFUSE)); shaderProgram.update("mixTexture", 1.0);}
	if (textures.find(aiTextureType_NORMALS) != textures.end() && shaderProgram.getUniformInfoMap()->find("normalTex") != shaderProgram.getUniformInfoMap()->end()) 
	{ shaderProgram.bindTextureOnUse("normalTex", textures.at(aiTextureType_NORMALS));shaderProgram.update("hasNormalTex", true);}

	DEBUGLOG->outdent();

	DEBUGLOG->log("FrameBufferObject Creation: GBuffer"); DEBUGLOG->indent();
	FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	FrameBufferObject gbufferFBO(shaderProgram.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default
	DEBUGLOG->outdent();

	DEBUGLOG->log("RenderPass Creation: GBuffer"); DEBUGLOG->indent();
	RenderPass renderGBuffer(&shaderProgram, &gbufferFBO);
	renderGBuffer.addEnable(GL_DEPTH_TEST);	
	renderGBuffer.setClearColor(0.0,0.0,0.0,0.0);
	renderGBuffer.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	for (auto r : objects){renderGBuffer.addRenderable(r);}  
	DEBUGLOG->outdent();

	// regular GBuffer compositing
	DEBUGLOG->log("Shader Compilation: GBuffer compositing"); DEBUGLOG->indent();
	ShaderProgram compShader("/screenSpace/fullscreen.vert", "/screenSpace/finalCompositing.frag"); DEBUGLOG->outdent();
	// set texture references
	compShader.bindTextureOnUse("colorMap", 	 gbufferFBO.getBuffer("fragColor"));
	compShader.bindTextureOnUse("normalMap", 	 gbufferFBO.getBuffer("fragNormal"));
	compShader.bindTextureOnUse("positionMap",  gbufferFBO.getBuffer("fragPosition"));

	FrameBufferObject compFBO(compShader.getOutputInfoMap(), WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);

	DEBUGLOG->log("RenderPass Creation: GBuffer Compositing"); DEBUGLOG->indent();
	Quad quad;
	RenderPass compositing(&compShader, &compFBO);
	compositing.addClearBit(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	compositing.setClearColor(0.25,0.25,0.35,0.0);
	compositing.addDisable(GL_DEPTH_TEST);
	compositing.addRenderable(&quad);
	DEBUGLOG->outdent();

	// Depth Of Field 
	PostProcessing::DepthOfField depthOfField(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y, &quad);
	
	// lens flare related stuff
	PostProcessing::SunOcclusionQuery sunOcclusionQuery(gbufferFBO.getDepthTextureHandle(), glm::vec2(gbufferFBO.getWidth(), gbufferFBO.getHeight()));
	PostProcessing::LensFlare lensFlare(gbufferFBO.getWidth() / 2, gbufferFBO.getHeight() / 2);

	// arbitrary texture display shader
	ShaderProgram showTexShader("/screenSpace/fullscreen.vert", "/screenSpace/simpleAlphaTexture.frag");
	RenderPass showTex(&showTexShader,0);
	showTex.addRenderable(&quad);
	showTex.addDisable(GL_DEPTH_TEST);
	showTex.addDisable(GL_BLEND);
	showTex.setViewport(0,0,WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);

	// arbitrary texture display shader
	ShaderProgram addTexShader("/screenSpace/fullscreen.vert", "/screenSpace/postProcessAddTexture.frag");
	RenderPass addTex(&addTexShader,0);
	addTex.addRenderable(&quad);
	addTex.addDisable(GL_DEPTH_TEST);
	addTex.addDisable(GL_BLEND);
	addTex.setViewport(0,0,WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);

	// misc display of sun
	ShaderProgram sunShader("/modelSpace/billboardProjection.vert", "/modelSpace/simpleColor.frag");
	sunShader.update("projection", perspective);
	//sunShader.update("color", glm::vec4(1.0,1.0,0.9,1.0));
	sunShader.bindTextureOnUse("tex", TextureTools::loadTextureFromResourceFolder("sun.png"));
	sunShader.update("blendColor", 1.0f);
	RenderPass sunPass(&sunShader, &compFBO);
	sunPass.addRenderable(&quad);
	sunPass.addEnable(GL_BLEND);
	sunPass.addEnable(GL_DEPTH_TEST);

	// Skybox
	GLuint cubeMapTexture = TextureTools::loadDefaultCubemap();

	// skybox rendering
	PostProcessing::SkyboxRendering skyboxRendering;
	skyboxRendering.m_skyboxShader.update("projection", perspective);

	// Blur stuff
	//PostProcessing::BoxBlur boxBlur(gbufferFBO.getWidth(), gbufferFBO.getHeight(),&quad);

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

	// 	ImGui_ImplGlfwGL3_MouseButtonCallback(window, b, a, m);
	};

	 auto keyboardCB = [&](int k, int s, int a, int m)
	 {
	 	if (a == GLFW_RELEASE) {return;} 
	 	switch (k)
	 	{
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

	 //model matrices / texture update function
	 std::function<void(Renderable*)> perRenderableFunction = [&](Renderable* r){ 
	 	static int i = 0;
	 	shaderProgram.update("model", turntable.getRotationMatrix() * modelMatrices[i] * glm::scale(s_scale));
	 	shaderProgram.update("color", colors[i]);
	 	i = (i+1)%modelMatrices.size();
	 	};
	 renderGBuffer.setPerRenderableFunction( &perRenderableFunction );

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////// RENDER LOOP /////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	double elapsedTime = 0.0;
	render(window, [&](double dt)
	{
		elapsedTime += dt;
		std::string window_header = "Post Processing Test - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
		ImGui::SliderFloat3("scale", glm::value_ptr(s_scale), 0.0f, 10.0f);

		ImGui::SliderFloat4("depths", glm::value_ptr(s_focusPlaneDepths), 0.0f, 10.0f);
		ImGui::SliderFloat2("radi", glm::value_ptr(s_focusPlaneRadi), -10.0f, 10.0f);
		ImGui::SliderFloat4("light", glm::value_ptr(s_light_position), -3.0f, 10.0f);
		
		ImGui::SliderFloat("far radius rescale", &s_farRadiusRescale, 0.0f, 5.0f);
		
	    ImGui::PushItemWidth(-150);
		if (ImGui::CollapsingHeader("lens flare"))
		{
			ImGui::SliderFloat("lens flare bias", &s_lensflare_bias, -2.0f, 2.0f);
			ImGui::SliderFloat("lens flare scale", &s_lensflare_scale, -5.0f, 5.0f);
			ImGui::SliderFloat("lens flare halo width", &s_lensflare_halo_width, 0.0f, 5.0f);
			ImGui::SliderInt("lens flare num ghosts", &s_lensflare_num_ghosts, 0, 10);
			ImGui::SliderFloat("lens flare ghost dispersal", &s_lensflare_ghost_dispersal, 0.0f, 5.0f);
			ImGui::SliderFloat("lens flare add strength", &s_lensflare_strength, 0.0f, 5.0f);
			ImGui::PopItemWidth();
		}
		//ImGui::SliderFloat("strength", &s_strength, 0.0f, 2.0f); // influence of color shift
		// ImGui::ColorEdit4( "color", glm::value_ptr( s_color)); // color mixed at max distance
       
		static bool s_dynamicDoF = false;
		ImGui::Checkbox("dynamic DoF", &s_dynamicDoF);
		
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// VARIABLE UPDATING ///////////////////////////////
		view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update view related uniforms
		shaderProgram.update( "view", view);
		shaderProgram.update( "color", s_color);

		//update light data
		glm::vec4 projectedLightPos = perspective * glm::mat4(glm::mat3(view)) * turntable.getRotationMatrix() * s_light_position;//multiply with the view-projection matrix
		projectedLightPos = projectedLightPos / projectedLightPos.w;//perform perspective division
		projectedLightPos.x = projectedLightPos.x*0.5+0.5;//the x/y screen coordinates come out between -1 and 1, so
		projectedLightPos.y = projectedLightPos.y*0.5+0.5;//we need to rescale them to be 0 to 1 tex-coords

		// debug rendering of a quad where the sun is
		sunShader.update("view", glm::mat4(glm::mat3(view))); // remove translation component
		sunShader.update("position", turntable.getRotationMatrix() * s_light_position);
		sunShader.update("scale", glm::vec3(0.1f, 0.1f * getRatio(window), 1.0f) );

		// skybox
		skyboxRendering.m_skyboxShader.update("view", glm::mat4(glm::mat3(view)) * turntable.getRotationMatrix());

		//compShader.update( "strength", s_strength);
		compShader.update("vLightPos", view * turntable.getRotationMatrix() * s_light_position);

		// update blurrying parameters
		depthOfField.m_calcCoCShader.update("focusPlaneDepths", s_focusPlaneDepths);
		depthOfField.m_calcCoCShader.update("focusPlaneRadi",   s_focusPlaneRadi );

		depthOfField.m_dofShader.update("maxCoCRadiusPixels", (int) s_focusPlaneRadi.x);
		depthOfField.m_dofShader.update("nearBlurRadiusPixels", (int) s_focusPlaneRadi.x/* / 4.0*/);
		depthOfField.m_dofShader.update("invNearBlurRadiusPixels", 1.0 / (s_focusPlaneRadi.x/* / 4.0*/));

		depthOfField.m_dofCompShader.update("maxCoCRadiusPixels", s_focusPlaneRadi.x);
		depthOfField.m_dofCompShader.update("farRadiusRescale" , s_farRadiusRescale);

		// lens flare parameters
		lensFlare.m_downSampleShader.update("uScale", glm::vec4(s_lensflare_scale));
		lensFlare.m_downSampleShader.update("uBias",  glm::vec4(s_lensflare_bias));
		lensFlare.m_ghostingShader.update("uGhosts", s_lensflare_num_ghosts);
		lensFlare.m_ghostingShader.update("uGhostDispersal", s_lensflare_ghost_dispersal);
		lensFlare.m_ghostingShader.update("uHaloWidth",  s_lensflare_halo_width);

		addTexShader.update("strength", s_lensflare_strength);
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
		renderGBuffer.render();

		GLuint pixelCount = sunOcclusionQuery.performQuery(projectedLightPos);

		// show light visibility in GUI
		ImGui::Value("sun visibility", (float) pixelCount / 256.0f);

		if ( s_dynamicDoF )
		{
			// read center depth
			gbufferFBO.bind();
			glm::vec4 value;
			// checkGLError();
			glReadBuffer(GL_COLOR_ATTACHMENT2);
			glReadPixels(gbufferFBO.getWidth() / 2, gbufferFBO.getHeight() /2, 1, 1,
			 GL_RGBA ,GL_FLOAT, glm::value_ptr(value) );
			// checkGLError();
			// DEBUGLOG->log("center depth:", value);
			float depth = glm::length(glm::vec3(value));

			float diffNear = (depth / 2.0f)-s_focusPlaneDepths.y;
			float diffFar =(depth + depth/2.0f)- s_focusPlaneDepths.z;
			s_focusPlaneDepths.x = s_focusPlaneDepths.x + diffNear * dt;
			s_focusPlaneDepths.y = s_focusPlaneDepths.y + diffNear * dt;
			s_focusPlaneDepths.z = s_focusPlaneDepths.z + diffFar * dt;
			s_focusPlaneDepths.w = s_focusPlaneDepths.w + diffFar * dt;
		}

		// aka. light pass
		compositing.render();

		// copy gbuffer depth buffer to compositing fbo
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gbufferFBO.getFramebufferHandle());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, compFBO.getFramebufferHandle());
		glBlitFramebuffer(0,0,gbufferFBO.getWidth(), gbufferFBO.getHeight(), 0,0,compFBO.getWidth(), compFBO.getHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		// render skybox
		skyboxRendering.render(cubeMapTexture, &compFBO);

		// render sun
		sunPass.render();

		// execute on GBuffer position texture and compositing ( light pass ) image 
		depthOfField.execute(gbufferFBO.getBuffer("fragPosition"), compFBO.getBuffer("fragmentColor"));

		// do it
		lensFlare.renderLensFlare(depthOfField.m_dofCompFBO->getBuffer("fragmentColor"));

		addTexShader.updateAndBindTexture("tex", 0, depthOfField.m_dofCompFBO->getBuffer("fragmentColor"));
		addTexShader.updateAndBindTexture("addTex", 1, lensFlare.m_featuresFBO->getBuffer("fResult"));
		addTex.render();
		
		// show result
		// showTex.setViewport(0,0,WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
		// showTexShader.updateAndBindTexture("tex", 0, depthOfField.m_dofCompFBO->getBuffer("fragmentColor"));
		// showTex.render();

		// showTex.setViewport(0,0,WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
		// showTexShader.updateAndBindTexture("tex", 0, lensFlare.m_featuresFBO->getBuffer("fResult"));
		// //showTexShader.updateAndBindTexture("tex", 0, sunOcclusionQuery.m_occlusionFBO->getBuffer("fragmentColor"));
		// //showTexShader.updateAndBindTexture("tex", 0, sunOcclusionFBO.getBuffer("fragmentColor"));
		// showTex.render();

		glViewport(0,0,WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);

		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}