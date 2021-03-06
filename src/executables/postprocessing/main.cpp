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

#include <Core/Camera.h>
#include <Rendering/CullingTools.h>

////////////////////// PARAMETERS /////////////////////////////
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);

static glm::vec4 s_color = glm::vec4(0.45 * 0.3f, 0.44f * 0.3f, 0.87f * 0.3f, 1.0f); // far : blueish
static glm::vec4 s_light_position = glm::vec4(-2.16f, 2.6f, 10.0f,1.0);
static glm::vec3 s_scale = glm::vec3(1.0f,1.0f,1.0f);

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
	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 30.f);

	// camera
	Camera camera;
	camera.setProjectionMatrix(perspective);
	Culling::CullingHelper cullingHelper(&camera);

	//objects
	std::vector<AssimpTools::RenderableInfo > objects;

	/////////////////////     Upload assets (create Renderables / VAOs from data)    //////////////////////////
	DEBUGLOG->log("Setup: creating VAOs from mesh data"); DEBUGLOG->indent();
	auto vertexData = AssimpTools::createVertexDataInstancesFromScene(scene);
	for (int i = 0; i < 5; i++)
	{
		auto renderables = AssimpTools::createSimpleRenderablesFromVertexDataInstances(vertexData);
		AssimpTools::RenderableInfo info;
		info.renderable = renderables[0];
		info.boundingBox = AssimpTools::computeBoundingBox(vertexData[0]);
		info.meshIdx = 0;
		info.name = "box_" + DebugLog::to_string(i);
		objects.push_back(info);
	}
	AssimpTools::RenderableInfo gridInfo;
	AssimpTools::BoundingBox gridBBox = {glm::vec3(5.0f,5.0,0.0f), glm::vec3(-5.0f,-5.0,0.0f)}; //max, min
	gridInfo.boundingBox = gridBBox;
	gridInfo.renderable = new Grid(10,10,1.0,1.0,true);
	gridInfo.name = "floor";
	gridInfo.meshIdx = -1;
	objects.push_back(gridInfo);
	
	//positions
	DEBUGLOG->log("Setup: model matrices"); DEBUGLOG->indent();
	std::unordered_map<Renderable*, glm::mat4> modelMatrices;
	modelMatrices[objects[0].renderable] = glm::translate(glm::vec3(randFloat(-3.0f,-2.0f), 0.0f, randFloat(-3,-2.0f)));
	modelMatrices[objects[1].renderable] = glm::translate(glm::vec3(randFloat(0.0f,1.0f),randFloat(2.0f,3.0f),randFloat(-5.0f,5.0f)));
	modelMatrices[objects[2].renderable] = glm::translate(glm::vec3(randFloat(-1.0f,1.0f),randFloat(-1.0f,0.0f),randFloat(0.0f,5.0f)));
	modelMatrices[objects[3].renderable] = glm::translate(glm::vec3(randFloat(0.5f,1.0f), randFloat(-5.0,-1.0),randFloat(0.0,5.0f)));
	modelMatrices[objects[4].renderable] = glm::translate(glm::vec3(randFloat(1.0f,5.0f), randFloat(0.0,2.0),randFloat(0.0,5.0f)));
	modelMatrices[objects[5].renderable] = glm::translate(glm::vec3(0.0f,-2.0f,0.0f)) * glm::rotate(glm::radians(90.0f), glm::vec3(1.0f,0.0,0.0));
	DEBUGLOG->outdent();

	//colors
	srand(time(NULL));
	std::unordered_map<Renderable*, glm::vec4> colors;
	colors[objects[0].renderable] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[objects[1].renderable] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[objects[2].renderable] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[objects[3].renderable] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[objects[4].renderable] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[objects[5].renderable] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);

	// cull info
	std::vector< std::pair< Renderable*, Culling::CullingInfo> >cullInfo;
	for( auto r : objects )
	{
		auto infoPair = Culling::getCullingInfo(r, modelMatrices[r.renderable] );
		cullInfo.push_back( infoPair );
	}

	// upload textures used by mesh
	std::unordered_map<aiTextureType, GLuint,AssimpTools::EnumClassHash> textures;
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
	shaderProgram.update("view", camera.getViewMatrix());
	shaderProgram.update("projection", camera.getProjectionMatrix());

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
	for (auto r : objects){renderGBuffer.addRenderable(r.renderable);}  
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

	// view from top
	ShaderProgram topViewShader("/modelSpace/modelViewProjection.vert", "/modelSpace/simpleColor.frag");
	topViewShader.printUniformInfo();
	topViewShader.update("projection", camera.getProjectionMatrix());
	topViewShader.update("view", glm::lookAt( glm::vec3(0,15.0f,0.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f, 0.0f, -1.0f) ));
	FrameBufferObject topViewFBO(topViewShader.getOutputInfoMap(), WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
	RenderPass renderTopView(&topViewShader, &topViewFBO);
	for (auto r : objects){renderTopView.addRenderable(r.renderable);}  
	renderTopView.addEnable(GL_DEPTH_TEST);
	renderTopView.addClearBit(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Skybox
	GLuint cubeMapTexture = TextureTools::loadDefaultCubemap();

	// skybox rendering
	PostProcessing::SkyboxRendering skyboxRendering;
	skyboxRendering.m_skyboxShader.update("projection", camera.getProjectionMatrix());

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
	
	static bool active_mouse_control = false;
	auto cursorPosCB = [&](double x, double y)
	{
	 	ImGuiIO& io = ImGui::GetIO();
	 	if ( io.WantCaptureMouse )
	 	{ return; } // ImGUI is handling this

		double d_x = x - old_x;
		double d_y = y - old_y;

		if ( active_mouse_control )
		{
			camera.mouseControlCallback(d_y,d_x);
		}
		if ( turntable.getDragActive() )
		{
			turntable.dragBy(d_x, d_y, camera.getViewMatrix());
		}

		old_x = x;
		old_y = y;
	};

	auto mouseButtonCB = [&](int b, int a, int m)
	{
		if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_PRESS)
		{
			active_mouse_control = true;
		}
		if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_RELEASE)
		{
			active_mouse_control = false;
		}
		if (b == GLFW_MOUSE_BUTTON_RIGHT && a == GLFW_PRESS)
		{
			turntable.setDragActive(true);
		}
		if (b == GLFW_MOUSE_BUTTON_RIGHT && a == GLFW_RELEASE)
		{
			turntable.setDragActive(false);
		}
	// 	ImGui_ImplGlfwGL3_MouseButtonCallback(window, b, a, m);
	};

	 auto keyboardCB = [&](int k, int s, int a, int m)
	 {
		 camera.keyboardControlCallback(k,s,a,m);
		 ImGui_ImplGlfwGL3_KeyCallback(window,k,s,a,m);
	 };

	setCursorPosCallback(window, cursorPosCB);
	setMouseButtonCallback(window, mouseButtonCB);
	setKeyCallback(window, keyboardCB);

	 //model matrices / texture update function
	std::function<void(Renderable*)> perRenderableFunction = [&](Renderable* r){ 
		shaderProgram.update("model", turntable.getRotationMatrix() * modelMatrices[r] * glm::scale(s_scale));
		shaderProgram.update("color", colors[r]);
	};
	renderGBuffer.setPerRenderableFunction( &perRenderableFunction );
	
	std::vector<Renderable* > visibleRenderables;
	std::function<void(Renderable*)> topViewPerRenderableFunction = [&](Renderable* r){ 
		topViewShader.update("model", turntable.getRotationMatrix() * modelMatrices[r] * glm::scale(s_scale));
		for ( auto vR : visibleRenderables)
		{
			if ( vR == r )
			{
				topViewShader.update("color", colors[r]);
				return;
			}
			topViewShader.update("color", colors[r] * 0.25f);
		}
	};
	renderTopView.setPerRenderableFunction( &topViewPerRenderableFunction );

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
		
		ImGui::PushItemWidth(-150);
		
		ImGui::SliderFloat4("light position", glm::value_ptr(s_light_position), -3.0f, 10.0f);
		ImGui::SliderFloat3("object scale", glm::value_ptr(s_scale), 0.0f, 10.0f);
		
		// Depth of field parameters
		static bool s_dynamicDoF = false;
		bool dof_collapsing_header = ImGui::CollapsingHeader("depth of field");
		if (dof_collapsing_header)
		{
			depthOfField.imguiInterfaceEditParameters();
			ImGui::Checkbox("dynamic DoF", &s_dynamicDoF);
		}
		if (s_dynamicDoF || dof_collapsing_header)
		{
			depthOfField.updateUniforms();
		}

		// Lens flare parameters
		if (ImGui::CollapsingHeader("lens flare"))
		{
			lensFlare.imguiInterfaceEditParameters();
			lensFlare.updateUniforms();
		}
		ImGui::PopItemWidth();

        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// VARIABLE UPDATING ///////////////////////////////
		//view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		camera.update(dt);
		//update light data
		glm::vec4 projectedLightPos = camera.getProjectionMatrix() * glm::mat4(glm::mat3(camera.getViewMatrix())) * s_light_position;//multiply with the view-projection matrix
		projectedLightPos = projectedLightPos / projectedLightPos.w;//perform perspective division
		projectedLightPos.x = projectedLightPos.x*0.5+0.5;//the x/y screen coordinates come out between -1 and 1, so
		projectedLightPos.y = projectedLightPos.y*0.5+0.5;//we need to rescale them to be 0 to 1 tex-coords
		//////////////////////////////////////////////////////////////////////////////
		
		/////////////////////////////   FRUSTUM CULLING    ///////////////////////////
		visibleRenderables = cullingHelper.cullAgainstFrustum(cullInfo);
		renderGBuffer.clearRenderables();
		for (auto r : visibleRenderables) { renderGBuffer.addRenderable(r); };
		ImGui::Value("visible objects: ", visibleRenderables.size());
		//////////////////////////////////////////////////////////////////////////////

		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update view related uniforms
		shaderProgram.update( "view", camera.getViewMatrix());
		shaderProgram.update( "color", s_color);

		// skybox
		skyboxRendering.m_skyboxShader.update("view", glm::mat4(glm::mat3(camera.getViewMatrix())));

		// light position
		compShader.update("vLightPos", camera.getViewMatrix() * s_light_position);

		// lens flare parameters
		lensFlare.updateLensStarMatrix(camera.getViewMatrix());
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
			glReadBuffer(GL_COLOR_ATTACHMENT2); // position buffer
			glReadPixels(gbufferFBO.getWidth() / 2, gbufferFBO.getHeight() /2, 1, 1,
			 GL_RGBA ,GL_FLOAT, glm::value_ptr(value) );
			// checkGLError();
			// DEBUGLOG->log("center depth:", value);
			float depth = glm::length(glm::vec3(value));

			float diffNear = (depth / 2.0f)-depthOfField.m_focusPlaneDepths.y;
			float diffFar =(depth + depth/2.0f)- depthOfField.m_focusPlaneDepths.z;
			depthOfField.m_focusPlaneDepths.x = depthOfField.m_focusPlaneDepths.x + diffNear * dt;
			depthOfField.m_focusPlaneDepths.y = depthOfField.m_focusPlaneDepths.y + diffNear * dt;
			depthOfField.m_focusPlaneDepths.z = depthOfField.m_focusPlaneDepths.z + diffFar * dt;
			depthOfField.m_focusPlaneDepths.w = depthOfField.m_focusPlaneDepths.w + diffFar * dt;
		}

		// aka. light pass
		compositing.render();

		// copy depth buffer content to compFBO to make sure Skybox is rendered correctly
		copyFBOContent(&gbufferFBO, &compFBO, GL_DEPTH_BUFFER_BIT);

		// render skybox
		skyboxRendering.render(cubeMapTexture, &compFBO);
		skyboxRendering.render(cubeMapTexture, &gbufferFBO);

		// execute on GBuffer position texture and compositing ( light pass ) image 
		depthOfField.execute(gbufferFBO.getBuffer("fragPosition"), compFBO.getBuffer("fragmentColor"));

		// do it
		lensFlare.renderLensFlare(depthOfField.m_dofCompFBO->getBuffer("fragmentColor"), 0);

		//addTexShader.updateAndBindTexture("tex", 0, depthOfField.m_dofCompFBO->getBuffer("fragmentColor"));
		////addTexShader.updateAndBindTexture("addTex", 1, lensFlare.m_featuresFBO->getBuffer("fResult"));
		//addTexShader.updateAndBindTexture("addTex", 1, lensFlare.m_boxBlur->m_mipmapTextureHandle);
		//addTex.render();
		
		// render top view
		renderTopView.render();

		// show / debug view of some texture
		showTex.setViewport(0,0,WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
		showTexShader.updateAndBindTexture("tex", 0, gbufferFBO.getBuffer("fragNormal"));
		showTex.render();

		showTex.setViewport(WINDOW_RESOLUTION.x / 4,0,WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
		showTexShader.updateAndBindTexture("tex", 0, topViewFBO.getBuffer("fragColor"));
		showTex.render();

		showTex.setViewport(WINDOW_RESOLUTION.x / 2,0,WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
		showTexShader.updateAndBindTexture("tex", 0, lensFlare.m_downSampleFBO->getBuffer("fResult"));
		showTex.render();

		showTex.setViewport(3 * WINDOW_RESOLUTION.x / 4,0,WINDOW_RESOLUTION.x / 4, WINDOW_RESOLUTION.y / 4);
		showTexShader.updateAndBindTexture("tex", 0, lensFlare.m_featuresFBO->getBuffer("fResult"));
		showTex.render();

		OPENGLCONTEXT->setViewport(0,0,WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);

		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}