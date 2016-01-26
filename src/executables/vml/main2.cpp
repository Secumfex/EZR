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
static bool s_switchCanvas = false;


const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);

void setupGLFWCallbacks() {

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
	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 100.0f);
    // setup light
    float lightIntensity = 70000000.0;
    glm::vec4 lightColor(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos(0.0f, 0.0f, 3.0f);
    glm::vec3 lightTarget(0,0,0);
    glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0,1,0));
    float lightViewAngle = 30.0f;
    float lightNearPlane = 1.0f;
    float lightFarPlane = 10.0f;
    glm::mat4 lightProjection = glm::perspective(glm::radians(lightViewAngle), getRatio(window), lightNearPlane, lightFarPlane);
    glm::mat4 lightMVP = lightProjection * lightView * model;
    // setup variables for raymarching
    int numberOfSamples = 7;
    float mediumDensity = 0.027;       // tau
    float scatterProbability = 0.02f;    // albedo
    // create objects
    float object_size = 0.5f;
	std::vector<Renderable* > objects;
    objects.push_back(new Volume(object_size));

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
    FrameBufferObject intSampCompBuffer(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
    intSampCompBuffer.addColorAttachments(1);
    FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default



    ///////////////////////    Shadowmapping   ///////////////////////////
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



    /////////////////////// 	Raymarching     //////////////////////////
    glm::vec4 cameraPosLightSpace = lightView * cameraPos;
    glm::vec4 cameraViewDirInvLightSpace = glm::normalize(lightView * (cameraPos - cameraTarget));

    DEBUGLOG->log("Shader Compilation: Raymarching shader"); DEBUGLOG->indent();
    ShaderProgram raymarchingShader("/vml/raymarching.vert", "/vml/raymarching_interleaved.frag");
    // uniforms for the vertex shader
    raymarchingShader.update("model", model);
    raymarchingShader.update("view", view);
    raymarchingShader.update("projection", perspective);
    raymarchingShader.update("lightView", lightView);
    // uniforms for the fragment shader
    raymarchingShader.bindTextureOnUse("shadowMapSampler", shadowMap.getDepthTextureHandle());
    raymarchingShader.update("numberOfSamples", numberOfSamples);
    raymarchingShader.update("phi", lightIntensity);
    raymarchingShader.update("lightProjection", lightProjection);
    raymarchingShader.update("lightColor", lightColor);
    raymarchingShader.update("cameraPosLightSpace", cameraPosLightSpace);
    //raymarchingShader.update("cameraViewDirInvLightSpace", cameraViewDirInvLightSpace);
    raymarchingShader.update("tau", mediumDensity);
    raymarchingShader.update("albedo", scatterProbability);
    DEBUGLOG->outdent();
    DEBUGLOG->log("Renderpass Creation: Raymarching renderpass"); DEBUGLOG->indent();
    RenderPass raymarchingRenderpass(&raymarchingShader, &volumeLightingBuffer);
    raymarchingRenderpass.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    raymarchingRenderpass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    raymarchingRenderpass.addEnable(GL_DEPTH_TEST);
    raymarchingRenderpass.addDisable(GL_BLEND);
    DEBUGLOG->outdent();
    // add objects to simple render pass
    objects.push_back(new Sphere(20, 40, 50.0f));
    for (auto r : objects )
    {
        raymarchingRenderpass.addRenderable(r);
    }



    /////////////////////// Interlaced Sampling //////////////////////////
    DEBUGLOG->log("Shader Compilation: Interleaved Sampling Composition shader"); DEBUGLOG->indent();
    ShaderProgram interleavedSamplingCompositionShader("/screenSpace/fullscreen.vert", "/vml/interleavedSamplingComposition.frag");
    interleavedSamplingCompositionShader.bindTextureOnUse("vliMap", volumeLightingBuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
    DEBUGLOG->outdent();
    DEBUGLOG->log("Renderpass Creation: Interleaved sampling composition renderpass"); DEBUGLOG->indent();
    RenderPass interleavedSamplingCompositionRenderpass(&interleavedSamplingCompositionShader, &intSampCompBuffer);
    interleavedSamplingCompositionRenderpass.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    interleavedSamplingCompositionRenderpass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    interleavedSamplingCompositionRenderpass.addEnable(GL_DEPTH_TEST);
    interleavedSamplingCompositionRenderpass.addDisable(GL_BLEND);
    DEBUGLOG->outdent();
    for (auto r : objects )
    {
        interleavedSamplingCompositionRenderpass.addRenderable(r);
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

    auto mouseScrollCB = [&](double xOffset, double yOffset)
    {
        cameraStartPos.z += (float) yOffset;

        //ImGui_ImplGlfwGL3_ScrollCallback(window, xOffset, yOffset);
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
            case GLFW_KEY_C:
                s_switchCanvas = !s_switchCanvas;
                break;
			default:
				break;
		}
		ImGui_ImplGlfwGL3_KeyCallback(window,k,s,a,m);
	};

    setScrollCallback(window, mouseScrollCB);
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
		std::string window_header = "Volume Lighting Renderer - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
		ImGui::PushItemWidth(-100);

        ImGui::SliderInt("Number ff Samples", &numberOfSamples, 1, 10);
        ImGui::SliderFloat("lightIntensity", &lightIntensity, 100.0f, 100000000.0f);
        ImGui::SliderFloat3("lightPos", glm::value_ptr(lightPos), -10.0f, 10.0f);
        ImGui::SliderFloat("lightAngle", &lightViewAngle, 0.1f, 90.0f);
        ImGui::SliderFloat("lightNearPlane", &lightNearPlane, 0.01f, 1.0f);
        ImGui::SliderFloat("lightFarPlane", &lightFarPlane, 10.0f, 1000.0f);
        ImGui::SliderFloat4("lightColor", glm::value_ptr(lightColor), 0.0f, 1.0f);
        ImGui::SliderFloat("tau", &mediumDensity, 0.0f, 1.0f);
        ImGui::SliderFloat("albedo", &scatterProbability, 0.0f, 1.0f);
		ImGui::Checkbox("auto-rotate", &s_isRotating); // enable/disable rotating volume
		ImGui::PopItemWidth();
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		if (s_isRotating) // update view matrix
		{
			model = glm::rotate(glm::mat4(1.0f), (float) dt, glm::vec3(0.0f, 1.0f, 0.0f) ) * model;
		}

        // update camera
        cameraPos = turntable.getRotationMatrix() * cameraStartPos;
		view = glm::lookAt(glm::vec3(cameraPos), glm::vec3(cameraTarget), glm::vec3(0.0f, 1.0f, 0.0f));

        // update the lightsource
        lightView = glm::lookAt(glm::vec3(lightPos), glm::vec3(lightTarget), glm::vec3(0,1,0));
        lightProjection = glm::perspective(glm::radians(lightViewAngle), getRatio(window), lightNearPlane, lightFarPlane);
        lightMVP = lightProjection * lightView * model;
        cameraPosLightSpace = lightView * cameraPos;
        cameraViewDirInvLightSpace = glm::normalize(lightView * (cameraPos - cameraTarget));

		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update shadowmap related uniforms
        shadowMapShader.update("lightMVP", lightMVP);

        // update raymarching related uniforms
        raymarchingShader.update("numberOfSamples", numberOfSamples);
        raymarchingShader.update("phi", lightIntensity);
        raymarchingShader.update("model", model);
        raymarchingShader.update("view", view);
        raymarchingShader.update("projection", perspective);
        raymarchingShader.update("lightView", lightView);
        raymarchingShader.update("lightProjection", lightProjection);
        raymarchingShader.update("lightColor", lightColor);
        raymarchingShader.update("cameraPosLightSpace", cameraPosLightSpace);
        raymarchingShader.update("tau", mediumDensity);
        raymarchingShader.update("albedo", scatterProbability);
//        raymarchingShader.update("cameraViewDirInvLightSpace", cameraViewDirInvLightSpace);
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //////////////////////////////////
        shadowMapRenderpass.render();
        raymarchingRenderpass.render();
        interleavedSamplingCompositionRenderpass.render();

        // setup vieports

        // setup small viewport for the shadowmap
        RenderPass showShadowMap( &texShader, 0 );
        showShadowMap.setViewport(0, 0, 150, 150);
        showShadowMap.addDisable(GL_BLEND);
        showShadowMap.addRenderable(&quad);

        // draw either of the two  renderpasses on the big viewport
        RenderPass showVolumeLighting( &texShader, 0 );
        RenderPass showInterSampComp( &texShader, 0 );
        if (s_switchCanvas) {
            // setup small viewport for the image
            showVolumeLighting.setViewport(150, 0, 150, 150);
            showVolumeLighting.addRenderable(&quad);

            // setup fullscreen viewport for the interleaved sampling
            showInterSampComp.addEnable(GL_BLEND);
            showInterSampComp.addClearBit(GL_COLOR_BUFFER_BIT);
            showInterSampComp.addRenderable(&quad);
        } else {
            // setup fullscreen viewport for the image
            showVolumeLighting.addClearBit(GL_COLOR_BUFFER_BIT);
            showVolumeLighting.addRenderable(&quad);

            // setup small viewport for the interleaved sampling
            showInterSampComp.setViewport(150, 0, 150, 150);
            showInterSampComp.addDisable(GL_BLEND);
            showInterSampComp.addRenderable(&quad);
        }

        // draw fbos on quad
        if (s_switchCanvas) {
            // render interleaved sampling composition to display
            texShader.bindTextureOnUse("tex", intSampCompBuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
            showInterSampComp.render();

            // render image in a small Viewport
            texShader.bindTextureOnUse("tex", volumeLightingBuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
            showVolumeLighting.render();
        } else {
            // render image to display
            texShader.bindTextureOnUse("tex", volumeLightingBuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
            showVolumeLighting.render();

            // render interleaved sampling composition in a small Viewport
            texShader.bindTextureOnUse("tex", intSampCompBuffer.getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
            showInterSampComp.render();
        }
        // render Shadowmap in a small Viewport
        texShader.bindTextureOnUse("tex", shadowMap.getDepthTextureHandle());
        showShadowMap.render();

        glViewport(0,0,WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
        ImGui::Render();
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}