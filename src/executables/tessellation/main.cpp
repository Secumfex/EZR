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

static glm::vec4 s_color = glm::vec4(0.45 * 0.3f, 0.44f * 0.3f, 0.87f * 0.3f, 1.0f); // far : blueish
static glm::vec4 s_lightPos = glm::vec4(2.0,2.0,2.0,1.0);

static float s_strength = 0.5f;

glm::mat4 bezier = glm::mat4(
	-1, 3, -3, 1,
	3, -6, 3, 0,
	-3, 3, 0, 0,
	1, 0, 0, 0
);

glm::mat4 bezier_transposed = glm::transpose(bezier);

const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);
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
	glm::vec4 eye(0.0f, 0.0f, 3.0f, 1.0f);
	glm::vec4 center(0.0f,0.0f,0.0f,1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0,1,0));

	 //glm::mat4 perspective = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 6.0f);
	/// perspective projection is experimental; yields weird warping effects due to vertex interpolation of uv-coordinates
	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 70.f);

	// object sizes
	float object_size = 0.25f;
	float ground_size = 3.0f;

	std::vector<Renderable* > objects;
	objects.push_back(new Terrain());

	GLuint distortionTex = TextureTools::loadTexture( RESOURCES_PATH "/terrain_height.png");
	GLuint diffTex = TextureTools::loadTexture( RESOURCES_PATH "/terrain_grass.jpg");
	GLuint rockTex = TextureTools::loadTexture( RESOURCES_PATH "/terrain_rock.jpg");

	DEBUGLOG->log("Setup: model matrices"); DEBUGLOG->indent();
	std::vector<glm::mat4 > modelMatrices;
	modelMatrices.resize(1);
	modelMatrices[0] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,-0.5f,0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f,0.0,0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(60.0f, 7.0f, 60.0f));
	glm::mat4 model = modelMatrices[0];
	DEBUGLOG->outdent();


	/////////////////////// 	Renderpasses     ///////////////////////////
	// regular GBuffer
	DEBUGLOG->log("Shader Compilation: GBuffer"); DEBUGLOG->indent();
	//ShaderProgram shaderProgram("/modelSpace/modelViewProjection.vert", "/tessellation/tess.frag", "/tessellation/tc.tesc", "/tessellation/te.tese", "/tessellation/tess_geom.geom"); DEBUGLOG->outdent();//
	ShaderProgram shaderProgram("/tessellation/test/test_vert.vert", "/tessellation/test/test_frag_lod.frag", "/tessellation/test/test_tc_lod.tc", "/tessellation/test/test_te_bezier.te"); DEBUGLOG->outdent();//
	shaderProgram.update("model", model);
	shaderProgram.update("view", view);
	shaderProgram.update("projection", perspective);
	shaderProgram.update("b", bezier);
	shaderProgram.update("bt", bezier_transposed);
	shaderProgram.bindTextureOnUse("terrain", distortionTex);
	shaderProgram.bindTextureOnUse("diff", diffTex);
	shaderProgram.bindTextureOnUse("rock", rockTex);

	//FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
	//FrameBufferObject fbo(shaderProgram.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
	//FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default

	//Quad quad;

	DEBUGLOG->log("RenderPass Creation: GBuffer"); DEBUGLOG->indent();
	RenderPass renderPass(&shaderProgram, 0);
	renderPass.addEnable(GL_DEPTH_TEST);
	renderPass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	renderPass.setClearColor(0.0, 0.0, 0.4,0.0);
	for (auto r : objects){renderPass.addRenderable(r);}

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

	// model matrices / texture update function
	std::function<void(Renderable*)> perRenderableFunction = [&](Renderable* r){ 
		static int i = 0;
		shaderProgram.update("model", turntable.getRotationMatrix() * modelMatrices[i]);

		if (i == 2) // is billboard
		{
		}

		i = (i+1)%modelMatrices.size();
		};
	renderPass.setPerRenderableFunction( &perRenderableFunction );

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////// RENDER LOOP /////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	double elapsedTime = 0.0;
	render(window, [&](double dt)
	{
		elapsedTime += dt;
		std::string window_header = "Tessellation - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
		
		ImGui::PushItemWidth(-100);

		ImGui::ColorEdit4( "color", glm::value_ptr( s_color)); // color mixed at max distance
        ImGui::SliderFloat("strength", &s_strength, 0.0f, 2.0f); // influence of color shift
        
		ImGui::Checkbox("auto-rotate", &s_isRotating); // enable/disable rotating volume
		ImGui::PopItemWidth();

        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		if (s_isRotating) // update view matrix
		{
			model = glm::rotate(glm::mat4(1.0f), (float) dt, glm::vec3(0.0f, 1.0f, 0.0f) ) * model;
		}

		view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));
		//////////////////////////////////////////////////////////////////////////////
				
		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////

		shaderProgram.update( "view", view);

		//////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////  RENDERING //// /////////////////////////////
		renderPass.render();

		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}