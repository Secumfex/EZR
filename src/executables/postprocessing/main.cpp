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
static glm::vec4 s_lightPos = glm::vec4(2.0,2.0,2.0,1.0);
static glm::vec4 s_focusPlaneDepths = glm::vec4(2.0,4.0,7.0,10.0);
static glm::vec2 s_focusPlaneRadi = glm::vec2(10.0f, -5.0f);
static glm::vec3 s_scale = glm::vec3(1.0f,1.0f,1.0f);
static float s_farRadiusRescale = 2.0f;
static float s_strength = 1.0f;
//////////////////// MISC /////////////////////////////////////
float randFloat(float min, float max) //!< returns a random number between min and max
{
	return (((float) rand() / (float) RAND_MAX) * (max - min) + min); 
}

void setLinearTextureFiltering(FrameBufferObject* fbo)
{	
	for ( auto t : fbo->getColorAttachments() )
	{
		glBindTexture(GL_TEXTURE_2D, t.second);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
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
	std::vector<glm::mat4> modelMatrices(7);
	modelMatrices[0] = glm::translate(glm::vec3(randFloat(-3.0f,-2.0f), 0.0f, randFloat(-3,-2.0f)));
	modelMatrices[1] = glm::translate(glm::vec3(randFloat(0.0f,1.0f),randFloat(2.0f,3.0f),randFloat(-5.0f,5.0f)));
	modelMatrices[2] = glm::translate(glm::vec3(randFloat(-1.0f,1.0f),randFloat(-1.0f,0.0f),randFloat(0.0f,5.0f)));
	modelMatrices[3] = glm::translate(glm::vec3(randFloat(0.5f,1.0f), randFloat(-5.0,-1.0),randFloat(0.0,5.0f)));
	modelMatrices[4] = glm::translate(glm::vec3(randFloat(1.0f,5.0f), randFloat(0.0,2.0),randFloat(0.0,5.0f)));
	modelMatrices[5] = glm::translate(glm::vec3(0.0f,-2.0f,0.0f)) * glm::rotate(glm::radians(90.0f), glm::vec3(1.0f,0.0,0.0));
	modelMatrices[6] = glm::scale( glm::vec3( 8.0f, 8.0f, 8.0f) );
	DEBUGLOG->outdent();

	srand(time(NULL));
	std::vector<glm::vec4> colors(7);
	colors[0] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[1] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[2] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[3] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[4] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[5] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);
	colors[6] = glm::vec4(randFloat(0.2f,1.0f), randFloat(0.2f,1.0f), randFloat(0.2f,1.0f),1.0f);

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
	objects.push_back(renderables[0]);
	
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
	// recenter view
	//center = glm::vec4(glm::vec3(center) + 0.5f * (bbox_max - bbox_min) + bbox_min, center.w);

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

	// render 
	DEBUGLOG->log("RenderPass Creation: Circle of Confusion Map RenderPass"); DEBUGLOG->indent();
	ShaderProgram calcCOCShader("/screenSpace/fullscreen.vert", "/screenSpace/postProcessCircleOfConfusion.frag");
	// calcCOCShader.bindTextureOnUse("depthMap", gbufferFBO.getDepthTextureHandle());
	calcCOCShader.bindTextureOnUse("positionMap", gbufferFBO.getBuffer("fragPosition"));
	calcCOCShader.bindTextureOnUse("colorMap",    compFBO.getBuffer("fragmentColor"));
	FrameBufferObject::s_internalFormat = GL_RGBA32F;
	FrameBufferObject cocFBO(calcCOCShader.getOutputInfoMap(), WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
	FrameBufferObject::s_internalFormat = GL_RGBA;
	RenderPass calcCOC(&calcCOCShader, &cocFBO);
	calcCOC.addRenderable(&quad);
	calcCOC.addDisable(GL_DEPTH_TEST);
	DEBUGLOG->outdent();

	DEBUGLOG->log("RenderPass Creation: Depth of Field Near/Far Texture Buffers"); DEBUGLOG->indent();
	ShaderProgram dofShader("/screenSpace/fullscreen.vert", "/screenSpace/postProcessDOF.frag");
	FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	FrameBufferObject hDofFBO(dofShader.getOutputInfoMap(), WINDOW_RESOLUTION.x / 4.0, WINDOW_RESOLUTION.y );
	FrameBufferObject vDofFBO(dofShader.getOutputInfoMap(), WINDOW_RESOLUTION.x / 4.0, WINDOW_RESOLUTION.y / 4.0);
	FrameBufferObject::s_internalFormat  = GL_RGBA; // to allow arbitrary values in G-Buffer
	// setLinearTextureFiltering(&hDofFBO);
	setLinearTextureFiltering(&vDofFBO);
	RenderPass depthOfField(&dofShader, &hDofFBO);
	depthOfField.addRenderable(&quad);
	depthOfField.addDisable(GL_DEPTH_TEST);
	DEBUGLOG->outdent();

	DEBUGLOG->log("RenderPass Creation: Depth of Field Compositing"); DEBUGLOG->indent();
	ShaderProgram dofCompShader("/screenSpace/fullscreen.vert", "/screenSpace/postProcessDOFCompositing.frag");
	dofCompShader.bindTextureOnUse("sharpFocusField", cocFBO.getBuffer("fragmentColor"));
	dofCompShader.bindTextureOnUse("blurryNearField", vDofFBO.getBuffer("nearResult"));
	dofCompShader.bindTextureOnUse("blurryFarField" , vDofFBO.getBuffer("blurResult"));
	FrameBufferObject dofCompFBO(dofCompShader.getOutputInfoMap(), WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y );
	RenderPass dofCompositing(&dofCompShader, &dofCompFBO);
	dofCompositing.addRenderable(&quad);
	dofCompositing.addDisable(GL_DEPTH_TEST);
	DEBUGLOG->outdent();

	ShaderProgram showTexShader("/screenSpace/fullscreen.vert", "/screenSpace/simpleAlphaTexture.frag");
	RenderPass showTex(&showTexShader,0);
	showTex.addRenderable(&quad);
	showTex.addDisable(GL_DEPTH_TEST);
	showTex.addDisable(GL_BLEND);
	showTex.setViewport(0,0,WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);

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
		
		ImGui::SliderFloat("far radius rescale", &s_farRadiusRescale, 0.0f, 5.0f);

	     //ImGui::SliderFloat("strength", &s_strength, 0.0f, 2.0f); // influence of color shift
    	// ImGui::PushItemWidth(-100);

		// ImGui::ColorEdit4( "color", glm::value_ptr( s_color)); // color mixed at max distance
        
		 //ImGui::Checkbox("auto-rotate", &s_isRotating); // enable/disable rotating volume
		 //ImGui::PopItemWidth();

        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update view related uniforms
		shaderProgram.update( "view", view);
		shaderProgram.update( "color", s_color);

		//compShader.update( "strength", s_strength);
		compShader.update("vLightPos", view * s_lightPos);

		// update blurrying parameters
		calcCOCShader.update("focusPlaneDepths", s_focusPlaneDepths);
		calcCOCShader.update("focusPlaneRadi",   s_focusPlaneRadi );

		dofShader.update("maxCoCRadiusPixels", (int) s_focusPlaneRadi.x);
		dofShader.update("nearBlurRadiusPixels", (int) s_focusPlaneRadi.x/* / 4.0*/);
		dofShader.update("invNearBlurRadiusPixels", 1.0 / (s_focusPlaneRadi.x/* / 4.0*/));

		dofCompShader.update("maxCoCRadiusPixels", s_focusPlaneRadi.x);
		dofCompShader.update("farRadiusRescale" , s_farRadiusRescale);
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
		renderGBuffer.render();

		// aka. light pass
		compositing.render();

		// from GBuffer
		calcCOC.render();

		// render once for horizontal blur
		depthOfField.setFrameBufferObject(&hDofFBO);
		depthOfField.setViewport(0, 0, hDofFBO.getWidth(), hDofFBO.getHeight());
		dofShader.update("HORIZONTAL", true);
		dofShader.updateAndBindTexture("blurSourceBuffer", 2, cocFBO.getBuffer("fragmentColor"));
		depthOfField.render();

		// render again for vertical blur
		depthOfField.setFrameBufferObject(&vDofFBO);
		depthOfField.setViewport(0, 0, vDofFBO.getWidth(), vDofFBO.getHeight());
		dofShader.update("HORIZONTAL", false);
		dofShader.updateAndBindTexture("blurSourceBuffer", 2, hDofFBO.getBuffer("blurResult"));
		dofShader.updateAndBindTexture("nearSourceBuffer", 3, hDofFBO.getBuffer("nearResult"));
		depthOfField.render();

		// blend results
		dofCompositing.render();

		// show result
		showTexShader.updateAndBindTexture("tex", 0, dofCompFBO.getBuffer("fragmentColor"));
		showTex.render();

		 ImGui::Render();
		 glDisable(GL_BLEND);
		 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}