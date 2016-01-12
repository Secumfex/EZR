/*******************************************
 * **** DESCRIPTION ****
 ****************************************/

#include <iostream>

#include <Rendering/GLTools.h>
#include <Rendering/VertexArrayObjects.h>
#include <Rendering/RenderPass.h>

#include "UI/imgui/imgui.h"
#include <UI/imguiTools.h>
#include <UI/Turntable.h>

#include <Importing/TextureTools.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

////////////////////// PARAMETERS /////////////////////////////
static bool s_isRotating = false;


//const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(1280.0f, 720.0f);
//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main()
{
	DEBUGLOG->setAutoPrint(true);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////// INIT //////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	// create window and opengl context
	auto window = generateWindow(WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// RENDERING  ///////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	
	/////////////////////     Scene / View Settings     //////////////////////////
    // set matrices
    glm::mat4 model = glm::mat4(1.0f);
    glm::vec4 cameraStartPos(0.0f, 0.0f, 3.0f, 1.0f);
	glm::vec4 cameraPos(0.0f, 0.0f, 3.0f, 1.0f);
	glm::vec4 cameraTarget(0.0f,0.0f,0.0f,1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(cameraPos), glm::vec3(cameraTarget), glm::vec3(0,1,0));
	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 50.f);

    // create objects
    float object_size = 0.4f;
	std::vector<Renderable* > objects;
    objects.push_back(new Volume(object_size));
    objects.push_back(new Sphere(20, 40, 40.0f));

	/////////////////////// 	Renderpass     ///////////////////////////

    ///////////////////////    Frambuffers     ///////////////////////////
    FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
    FrameBufferObject frameBuffer(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
    frameBuffer.addColorAttachments(1);
    FrameBufferObject debugBuffer(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
    debugBuffer.addColorAttachments(1);
    FrameBufferObject shadowMap(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
    FrameBufferObject volumeLightingBuffer(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
    volumeLightingBuffer.addColorAttachments(1);
    FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default

    ///////////////////////    Shadowmapping   ///////////////////////////
    // setup light
    glm::vec4 lightColor(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos(0.0f, 5.0f, 5.0f);
    glm::vec3 lightCenter(0,0,0);
    glm::mat4 lightView = glm::lookAt(lightPos, lightCenter, glm::vec3(0,1,0));
    glm::mat4 lightProjection = glm::perspective(glm::radians(5.0f), getRatio(window), 0.10f, 100.0f);
    glm::mat4 lightMVP = lightProjection * lightView * model;

    // load and update shadowmap shader
    DEBUGLOG->log("Shader Compilation: shadowmap shader"); DEBUGLOG->indent();
    ShaderProgram shadowMapShader("/vml/shadowmap.vert", "/vml/shadowmap.frag");
    shadowMapShader.update("lightMVP", lightMVP);
    DEBUGLOG->outdent();

    // setup shadowmap renderpass
    DEBUGLOG->log("Renderpass Creation: simple renderpass"); DEBUGLOG->indent();
    RenderPass shadowMapRenderpass(&shadowMapShader, &shadowMap);
    shadowMapRenderpass.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    shadowMapRenderpass.addClearBit(GL_DEPTH_BUFFER_BIT);
    shadowMapRenderpass.addEnable(GL_DEPTH_TEST);
    DEBUGLOG->outdent();

    // add objects to shadowMap render pass
    for (auto r : objects )
    {
        shadowMapRenderpass.addRenderable(r);
    }

    /////////////////////// 	Light Shading     //////////////////////////
    DEBUGLOG->log("Shader Compilation: Light shader"); DEBUGLOG->indent();
    ShaderProgram lightShader("/vml/simple.vert", "/vml/simple.frag");
    lightShader.update("model", glm::translate(model, lightPos));
    lightShader.update("view", view);
    lightShader.update("projection", perspective);
    lightShader.update("color", lightColor);
    DEBUGLOG->outdent();

    DEBUGLOG->log("Renderpass Creation: simple renderpass"); DEBUGLOG->indent();
    RenderPass lightRenderpass(&lightShader, &debugBuffer);
    lightRenderpass.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    lightRenderpass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    lightRenderpass.addEnable(GL_DEPTH_TEST);
    DEBUGLOG->outdent();

    // add light
    lightRenderpass.addRenderable(new Sphere(20,40,0.25f));

    /////////////////////// 	Raymarching     //////////////////////////
    glm::vec4 cameraPosLightSpace = lightView * cameraPos;
    glm::vec4 cameraViewDirInvLightSpace = glm::normalize(lightView * (cameraPos - cameraTarget));

    DEBUGLOG->log("Shader Compilation: Raymarching shader"); DEBUGLOG->indent();
    ShaderProgram raymarchingShader("/vml/raymarching.vert", "/vml/raymarching.frag");
    // uniforms for the vertex shader
    raymarchingShader.update("model", model);
    raymarchingShader.update("view", view);
    raymarchingShader.update("projection", perspective);
    raymarchingShader.update("lightView", lightView);
    // uniforms for the fragment shader
    raymarchingShader.bindTextureOnUse("shadowMapSampler", shadowMap.getDepthTextureHandle());
    raymarchingShader.update("lightProjection", lightProjection);
    raymarchingShader.update("lightColor", lightColor);
    raymarchingShader.update("cameraPosLightSpace", cameraPosLightSpace);
    raymarchingShader.update("cameraViewDirInvLightSpace", cameraViewDirInvLightSpace);
    DEBUGLOG->outdent();

    DEBUGLOG->log("Renderpass Creation: Raymarching renderpass"); DEBUGLOG->indent();
    RenderPass raymarchingRenderpass(&raymarchingShader, &volumeLightingBuffer);
    raymarchingRenderpass.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    raymarchingRenderpass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    raymarchingRenderpass.addEnable(GL_DEPTH_TEST);
    raymarchingRenderpass.addDisable(GL_BLEND);
    DEBUGLOG->outdent();

    // add objects to simple render pass
    for (auto r : objects )
    {
        raymarchingRenderpass.addRenderable(r);
    }

    /////////////////////// 	Compositing     //////////////////////////
    Quad quad;
    ShaderProgram texShader("/screenSpace/fullscreen.vert", "/screenSpace/simpleAlphaTexture.frag");

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
		switch (k)
		{
			case GLFW_KEY_W:
				cameraPos += glm::inverse(view)    * glm::vec4(0.0f,0.0f,-0.1f,0.0f);
				cameraTarget += glm::inverse(view) * glm::vec4(0.0f,0.0f,-0.1f,0.0f);
				break;
			case GLFW_KEY_A:
				cameraPos += glm::inverse(view)	 * glm::vec4(-0.1f,0.0f,0.0f,0.0f);
				cameraTarget += glm::inverse(view) * glm::vec4(-0.1f,0.0f,0.0f,0.0f);
				break;
			case GLFW_KEY_S:
				cameraPos += glm::inverse(view)    * glm::vec4(0.0f,0.0f,0.1f,0.0f);
				cameraTarget += glm::inverse(view) * glm::vec4(0.0f,0.0f,0.1f,0.0f);
				break;
			case GLFW_KEY_D:
				cameraPos += glm::inverse(view)    * glm::vec4(0.1f,0.0f,0.0f,0.0f);
				cameraTarget += glm::inverse(view) * glm::vec4(0.1f,0.0f,0.0f,0.0f);
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
        // set window title
		elapsedTime += dt;
		std::string window_header = "Simple Renderer - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
		ImGui::PushItemWidth(-100);

        ImGui::SliderFloat3("lightPos", glm::value_ptr(lightPos), -10.0f, 10.0f);
        ImGui::SliderFloat4("lightColor", glm::value_ptr(lightColor), 0.0f, 1.0f);
		ImGui::Checkbox("auto-rotate", &s_isRotating); // enable/disable rotating volume
		ImGui::PopItemWidth();
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		if (s_isRotating) // update view matrix
		{
			model = glm::rotate(glm::mat4(1.0f), (float) dt, glm::vec3(0.0f, 1.0f, 0.0f) ) * model;
		}

        // update view matrix (camera pos)
        cameraPos = turntable.getRotationMatrix() * cameraStartPos;
		view = glm::lookAt(glm::vec3(cameraPos), glm::vec3(cameraTarget), glm::vec3(0.0f, 1.0f, 0.0f));

        // update the lightsource
        lightView = glm::lookAt(glm::vec3(lightPos), glm::vec3(lightCenter), glm::vec3(0,1,0));
        cameraPosLightSpace = lightView * cameraPos;
        cameraViewDirInvLightSpace = glm::normalize(lightView * (cameraPos - cameraTarget));
        lightMVP = lightProjection * lightView * model;

		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update shadowmap related uniforms
        shadowMapShader.update("lightMVP", lightMVP);

        // update lightShader uniforms
        lightShader.update( "color", lightColor);
        lightShader.update( "view", view);
        lightShader.update( "model", glm::translate(glm::mat4(1.0f), lightPos));

        // update raymarching related uniforms
        raymarchingShader.update("model", model);
        raymarchingShader.update("view", view);
        raymarchingShader.update("projection", perspective);
        raymarchingShader.update("lightView", lightView);
        raymarchingShader.update("lightProjection", lightProjection);
        raymarchingShader.update("lightColor", lightColor);
        raymarchingShader.update("cameraPosLightSpace", cameraPosLightSpace);
        raymarchingShader.update("cameraViewDirInvLightSpace", cameraViewDirInvLightSpace);
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
        shadowMapRenderpass.render();
        raymarchingRenderpass.render();
        lightRenderpass.render();

        // setup fullscreen viewport for the image
        RenderPass showVolumeLighting( &texShader, 0 );
        showVolumeLighting.addClearBit(GL_COLOR_BUFFER_BIT);
        showVolumeLighting.addRenderable(&quad);

        // setup small viewport for the shadowmap
        RenderPass showShadowMap( &texShader, 0 );
        showShadowMap.setViewport(0, 0, 150, 150);
        showShadowMap.addDisable(GL_BLEND);
        showShadowMap.addRenderable(&quad);

        // setup small viewport for the shadowmap
        RenderPass showLight( &texShader, 0 );
        showLight.setViewport(150, 0, 150, 150);
        showLight.addDisable(GL_BLEND);
        showLight.addRenderable(&quad);

        // render image to display
        texShader.bindTextureOnUse("tex", volumeLightingBuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
        showVolumeLighting.render();

        // render Shadowmap in a small Viewport
        texShader.bindTextureOnUse("tex", shadowMap.getDepthTextureHandle());
        showShadowMap.render();

        // render lightposition in a small Viewport
        texShader.bindTextureOnUse("tex", debugBuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
        showLight.render();

        glViewport(0,0,WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
        ImGui::Render();
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}