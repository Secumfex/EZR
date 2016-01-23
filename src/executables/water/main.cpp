/*******************************************
 * **** DESCRIPTION ****
 ****************************************/

////////////////////// INCLUDES /////////////////////////////
#include <iostream>
#include <time.h>

#include <Rendering/GLTools.h>
#include <Rendering/VertexArrayObjects.h>
#include <Rendering/RenderPass.h>

#include "UI/imgui/imgui.h"
#include <UI/imguiTools.h>
#include <UI/Turntable.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <Importing/AssimpTools.h>
#include <Importing/TextureTools.h>

////////////////////// PARAMETERS /////////////////////////////
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);

//.....

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main()
{
	DEBUGLOG->setAutoPrint(true);
	auto window = generateWindow(WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////// INIT /////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	// import using ASSIMP and check for errors
	Assimp::Importer importer;
	DEBUGLOG->log("Loading branch model");
	std::string terrainModel = "mountains.obj";
	const aiScene* scene = AssimpTools::importAssetFromResourceFolder(terrainModel, importer);
	glm::mat4 transform = glm::scale( glm::vec3(1, 1, 1) );
	Renderable* assimpRenderableTerrain = AssimpTools::createSimpleRenderablesFromScene(terrainModel, transform);

	//.....
// neues render target erstellen (reflection)
// neues render target erstellen (refraction)

// bump map für wellen

	//////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// RENDERING  ///////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	/////////////////////     Scene / View Settings     //////////////////////////
	glm::mat4 model = glm::mat4(1.0f);
	glm::vec4 eye(0.0f, 0.0f, 3.0f, 1.0f);
	glm::vec4 center(0.0f,0.0f,0.0f,1.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0,1,0));

	glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 10.f);

	//object sizes
	float groundSize = 3.0f;

	std::vector<Renderable* > objects;
	objects.push_back(new Grid(1,1,groundSize,groundSize,true));
	objects.push_back(assimpRenderableTerrain);

	/////////////////////// 	Renderpasses     ///////////////////////////
	//normaler pass
	//set shader
	DEBUGLOG->log("Shader Compilation: GBuffer"); DEBUGLOG->indent();
	ShaderProgram shaderProgram("/xx/xx.vert", "/xx/xx.frag"); DEBUGLOG->outdent();
	//update vars
	//update tex/sampler
	//create FBO
	//create render pass
	DEBUGLOG->log("RenderPass Creation: GBuffer"); DEBUGLOG->indent();
	RenderPass renderPass(&shaderProgram, &fbo);
	renderPass.addEnable(GL_DEPTH_TEST);
	// renderPass.addEnable(GL_BLEND);
	renderPass.setClearColor(0.0,0.0,0.0,0.0);
	renderPass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	for (auto r : objects){renderPass.addRenderable(r);}

	//1. wasser pass (reflec+refrac)
	//set shader
	DEBUGLOG->log("Shader Compilation: ReflectionShader"); DEBUGLOG->indent();
	ShaderProgram reflShader("/xx/xx.vert", "/xx/xx.frag");	DEBUGLOG->outdent();

	//update vars

	//update tex/sampler

	//create FBO
	DEBUGLOG->log("FrameBufferObject Creation: waterFBO"); DEBUGLOG->indent();
	FrameBufferObject waterFBO(getResolution(window).x, getResolution(window).y);
	waterFBO.addColorAttachments(2); DEBUGLOG->outdent();

	//create render pass
	DEBUGLOG->log("RenderPass Creation: Reflection"); DEBUGLOG->indent();
	RenderPass reflRenderPass(&reflShader, &waterFBO );
	reflRenderPass.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	reflRenderPass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	//2. wasser pass (kombination)
	//set shader
	DEBUGLOG->log("Shader Compilation: waterShader"); DEBUGLOG->indent();
	ShaderProgram waterShader("/waterReflections/water.vert", "/waterReflections/water.frag");	DEBUGLOG->outdent();

	//update vars
	waterShader.update();
	//...

	//update tex/sampler
	waterShader.bindTextureOnUse();
	//...

	//create render pass
	DEBUGLOG->log("RenderPass Creation: Combine"); DEBUGLOG->indent();
	RenderPass combRenderPass(&waterShader, &waterFBO );

	//.....
	// todo: entsprechend abändern/verschieben
	/*void renderReflection(){
		glViewport(0,0, texSize, texSize);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		gluLookAt(......)

		glPushMatrix();
		glTranslatef(0.0f, 0.0f, 0.0f);
		glScalef(1.0, -1.0, 1.0);
		double plane[4] = {0.0, 1.0, 0.0, 0.0}; //water at y=0
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE0, plane);
		RenderScene();
		glDisable(GL_CLIP_PLANE0);
		glPopMatrix();

		//render reflection to texture
		glBindTexture(GL_TEXTURE_2D, reflection);
		//glCopyTexSubImage2D copies the frame buffer
		//to the bound texture
		glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,texSize, texSize);
	}

	// todo: entsprechend abändern/verschieben
	void renderRefractionAndDepth(){
		glViewport(0,0, texSize, texSize);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		gluLookAt(......)

		glPushMatrix();
		glTranslatef(0.0f, 0.0f, 0.0f);
		//normal pointing along negative y
		double plane[4] = {0.0, -1.0, 0.0, 0.0};
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE0, plane);
		RenderScene();
		glDisable(GL_CLIP_PLANE0);
		glPopMatrix();

		//render color buffer to texture
		glBindTexture(GL_TEXTURE_2D, refraction);
		glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,texSize, texSize);

		//render depth to texture
		glBindTexture(GL_TEXTURE_2D, depth);
		glCopyTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT, 0,0, texSize,texSize, 0);
	}

	// todo: entsprechend abändern/verschieben
	void renderWater(){
		// s. 5-7
	}
	*/

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
		switch(k){
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
		elapsedTime += dt;
		std::string window_header = "Water Test - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
		ImGui::PushItemWidth(-125);

		static glm::vec3 angleshifts[3] ={glm::vec3(0.0),glm::vec3(0.0),glm::vec3(0.0)};
		ImGui::SliderFloat3("vAngleShiftFront", glm::value_ptr( angleshifts[0]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAngleShiftBack", glm::value_ptr( angleshifts[1]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAngleShiftSide", glm::value_ptr( angleshifts[2]), -1.0f, 1.0f);

		static glm::vec3 amplitudes[3] = {glm::vec3(1.0),glm::vec3(1.0),glm::vec3(1.0)};
		ImGui::SliderFloat3("vAmplitudesFront", glm::value_ptr( amplitudes[0]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAmplitudesBack", glm::value_ptr( amplitudes[1]), -1.0f, 1.0f);
		ImGui::SliderFloat3("vAmplitudesSide", glm::value_ptr( amplitudes[2]), -1.0f, 1.0f);

		static glm::vec3 frequencies(1.0f);
		ImGui::SliderFloat3("fFrequencies", glm::value_ptr( frequencies), 0.0f, 3.0f);

		static float tree_phase = 0.0f;
		ImGui::SliderFloat("treePhase", &tree_phase, 0.0, glm::two_pi<float>());

		ImGui::PopItemWidth();
        //////////////////////////////////////////////////////////////////////////////

		///////////////////////////// MATRIX UPDATING ///////////////////////////////
		view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0.0f, 1.0f, 0.0f));

		// view spiegeln (reflection)
		// clipping plane
		// draw into render target

		// transform clipping plane (refraction)
		// draw into render target

		// fresnel term um beide zu überblenden
		//    mehrere mögl. (s. 9)

		//////////////////////////////////////////////////////////////////////////////

		////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
		// update view related uniforms
		shaderProgram.update();
		//.....

		////////////////////////////////  RENDERING //// /////////////////////////////
		/*renderPass.render();

		compositing.render();
		*/
		renderPass.render();
		reflRenderPass.render();
		combRenderPass.render();


		ImGui::Render();
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
		//////////////////////////////////////////////////////////////////////////////

	});

	destroyWindow(window);

	return 0;
}
