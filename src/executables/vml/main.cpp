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

static glm::vec4 s_color = glm::vec4(0.45, 0.44f, 0.87f, 1.0f); // far : blueish
static glm::vec4 s_lightPos = glm::vec4(2.0,2.0,2.0,1.0);

static float s_strength = 0.05f;
static float s_transparency = 0.05f;

const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);
//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void composition(FrameBufferObject frame, FrameBufferObject shadowmap) {
    // Create the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frame.getFramebufferHandle());
    GLsizei halfWidth = (GLsizei) (WINDOW_RESOLUTION.x / 2.0f);
    GLsizei halfHeight = (GLsizei) (WINDOW_RESOLUTION.y / 2.0f);

    // draw renderpass to full screen
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y,
                      0, 0, WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

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
	glm::vec4 eye(0.0f, 0.0f, 3.0f, 1.0f);
	glm::vec4 center(0.0f,0.0f,0.0f,1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0,1,0));
	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 1000.f);

    // create objects
    float object_size = 1.0f;
	std::vector<Renderable* > objects;
    objects.push_back(new Volume(object_size));

	/////////////////////// 	Renderpass     ///////////////////////////

    ///////////////////////    Shadowmapping   ///////////////////////////
    // setup shadowmap
    FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
    FrameBufferObject frameBuffer(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
    FrameBufferObject shadowMap(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
    FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default

    // setup light
    glm::vec3 lightPos(5, 20, 20);
    glm::vec3 lightInvViewDir = glm::vec3(0.5f, 2, 2);
    glm::mat4 lightProjection = glm::perspective(45.0f, 1.0f, 2.0f, 50.0f);
    glm::mat4 lightViewMatrix = glm::lookAt(lightPos, lightPos-lightInvViewDir, glm::vec3(0,1,0));
    glm::mat4 lightModelMatrix = glm::mat4(1.0);
    glm::mat4 lightMVP = lightProjection * lightViewMatrix * lightModelMatrix;

    DEBUGLOG->log("Shader Compilation: shadowmap shader"); DEBUGLOG->indent();
    ShaderProgram shadowMapShader("/vml/shadowmap.vert", "/vml/shadowmap.frag");
    shadowMapShader.update("lightMVP", lightMVP);
    DEBUGLOG->outdent();

    DEBUGLOG->log("Renderpass Creation: simple renderpass"); DEBUGLOG->indent();
    RenderPass shadowMapRenderpass(&shadowMapShader, &shadowMap);
    shadowMapRenderpass.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    shadowMapRenderpass.addClearBit(GL_DEPTH_BUFFER_BIT);
    shadowMapRenderpass.addEnable(GL_DEPTH_TEST);
    DEBUGLOG->outdent();

    // add objects to simple render pass
    for (auto r : objects )
    {
        shadowMapRenderpass.addRenderable(r);
    }

    /////////////////////// 	Raymarching     //////////////////////////
	DEBUGLOG->log("Shader Compilation: simple shader"); DEBUGLOG->indent();
	ShaderProgram simpleShader("/vml/simple.vert", "/vml/simple.frag");
	simpleShader.update("model", model);
	simpleShader.update("view", view);
	simpleShader.update("projection", perspective);
	simpleShader.update("color", s_color);
	//simpleShader.update("texResolution", WINDOW_RESOLUTION);
	DEBUGLOG->outdent();

	DEBUGLOG->log("Renderpass Creation: simple renderpass"); DEBUGLOG->indent();
	RenderPass simpleRenderpass(&simpleShader, &frameBuffer);
	simpleRenderpass.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	simpleRenderpass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	simpleRenderpass.addEnable(GL_DEPTH_TEST);
	DEBUGLOG->outdent();

	// add objects to simple render pass
	for (auto r : objects )
	{
		simpleRenderpass.addRenderable(r);
	}

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
        // set window title
		elapsedTime += dt;
		std::string window_header = "Simple Renderer - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
		
		ImGui::PushItemWidth(-100);

/*		ImGui::ColorEdit4( "color", glm::value_ptr( s_color)); // color mixed at max distance
        ImGui::SliderFloat("strength", &s_strength, 0.0f, 2.0f); // influence of color shift
        ImGui::SliderFloat("transparency", &s_transparency, 0.0f, 1.0f); // influence of color shift
        
		ImGui::Checkbox("auto-rotate", &s_isRotating); // enable/disable rotating volume
		ImGui::PopItemWidth();*/
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		if (s_isRotating) // update view matrix
		{
			model = glm::rotate(glm::mat4(1.0f), (float) dt, glm::vec3(0.0f, 1.0f, 0.0f) ) * model;
		}

		view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update shadowmap related uniforms
        shadowMapShader.update("lightMVP", lightMVP);

        // update view related uniforms
		simpleShader.update( "color", s_color);
		simpleShader.update( "view", view);
		simpleShader.update( "model", turntable.getRotationMatrix() * model);
		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
        shadowMapRenderpass.render();
        simpleRenderpass.render();
        composition(frameBuffer, shadowMap);

		ImGui::Render();
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}