/*******************************************
 * **** DESCRIPTION ****
 ****************************************/

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

#include <TreeAnimation/Tree.h>
#include <TreeAnimation/TreeRendering.h>
#include <TreeAnimation/WindField.h>

////////////////////// PARAMETERS /////////////////////////////
const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);

.....

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

.....
// neues render target erstellen (reflection)
// neues render target erstellen (refraction)

// bump map für wellen

	//////////////////////////////////////////////////////////////////////////////
		/////////////////////////////// RENDERING  ///////////////////////////////////
		//////////////////////////////////////////////////////////////////////////////

		/////////////////////     Scene / View Settings     //////////////////////////
		glm::vec4 eye(0.0f, 0.0f, 3.0f, 1.0f);
		glm::vec4 center(0.0f,0.0f,0.0f,1.0f);
		glm::mat4 view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(0,1,0));

		glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 10.f);

		DEBUGLOG->log("Setup: model matrices"); DEBUGLOG->indent();
		glm::mat4 model = glm::translate(glm::vec3(0.0f, - tree_height / 2.0f, 0.0f));
		//glm::mat4 model = glm::mat4(1.0f);
		DEBUGLOG->outdent();

		/////////////////////// 	Renderpasses     ///////////////////////////

.....

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
		s_simulationTime = elapsedTime;
		std::string window_header = "Tree Animation Test - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
		glfwSetWindowTitle(window, window_header.c_str() );

		////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
		ImGui::PushItemWidth(-125);

		ImGui::SliderFloat("windDirection", &s_wind_angle, 0.0f, 360.0f);
		ImGui::SliderFloat("windPower", &s_wind_power, 0.0f, 4.0f);

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
		shaderProgram.update( "view",  view);
		shaderProgram.update( "model", turntable.getRotationMatrix() * model);

		shaderProgram.update("simTime", s_simulationTime);
		s_wind_direction = glm::rotateY(glm::vec3(0.0f,0.0f,1.0f), glm::radians(s_wind_angle));
		shaderProgram.update( "windDirection", s_wind_direction);

		glm::vec3 windTangent = glm::vec3(-s_wind_direction.z, s_wind_direction.y, s_wind_direction.x);
		float animatedWindPower = sin(s_simulationTime) * (s_wind_power / 2.0f) + s_wind_power / 2.0f + (0.25f * sin(2.0f * s_wind_power * s_simulationTime + 0.25f)) ;
		s_wind_rotation = glm::rotate(glm::mat4(1.0f), (animatedWindPower / 2.0f), windTangent);
		shaderProgram.update( "windRotation" , s_wind_rotation);

		shaderProgram.update("vAngleShiftFront", angleshifts[0]); //front
		shaderProgram.update("vAngleShiftBack", angleshifts[1]); //back
		shaderProgram.update("vAngleShiftSide", angleshifts[2]); //side

		shaderProgram.update("vAmplitudesFront", amplitudes[0]); //front
		shaderProgram.update("vAmplitudesBack", amplitudes[1]); //back
		shaderProgram.update("vAmplitudesSide", amplitudes[2]); //side

		shaderProgram.update("fFrequencyFront", frequencies.x); //front
		shaderProgram.update("fFrequencyBack", frequencies.y); //back
		shaderProgram.update("fFrequencySide", frequencies.z); //side

		shaderProgram.update("tree.phase", tree_phase); //front

		.....

		////////////////////////////////  RENDERING //// /////////////////////////////
				renderPass.render();

				compositing.render();

				ImGui::Render();
				glDisable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // this is altered by ImGui::Render(), so reset it every frame
				//////////////////////////////////////////////////////////////////////////////

			});

			destroyWindow(window);

			return 0;
		}
