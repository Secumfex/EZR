#ifndef MISC_CPP
#define MISC_CPP

#include "UI/imgui/imgui.h"
#include <UI/imguiTools.h>
#include <functional>
#include <Core/Camera.h>
#include <Rendering/GLTools.h>

/***********************************************/
// This file is for arbitrary stuff to save some ugly Lines of Code
/***********************************************/

/***********************************************/
namespace CallbackHelper
{
	static double old_x;
	static double old_y;
	static double d_x;
	static double d_y;

	static std::function<void (double, double)> cursorPosFunc;
	static std::function<void (int, int, int)> mouseButtonFunc;
	static std::function<void (int,int,int,int)> keyboardFunc;

	static bool active_mouse_control = false;

	static Camera* mainCamera = nullptr;
	static GLFWwindow* window;

	inline void cursorPosCallback(double x, double y)
	{
		ImGuiIO& io = ImGui::GetIO();
	 	if ( io.WantCaptureMouse )
	 	{ return; } // ImGUI is handling this

		d_x = x - old_x;
		d_y = y - old_y;

		if ( active_mouse_control  && mainCamera != nullptr)
		{
			mainCamera->mouseControlCallback(d_y, d_x);
		}

		cursorPosFunc(x,y); // inner stuff from main()

		old_x = x;
		old_y = y;
	}

	inline void mouseButtonCallback(int b, int a, int m)
	{
		if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_PRESS)
		{
			active_mouse_control = true;
		}
		if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_RELEASE)
		{
			active_mouse_control = false;
		}

		mouseButtonFunc(b, a, m);

	 	ImGui_ImplGlfwGL3_MouseButtonCallback(window, b, a, m);
	}

	inline void keyboardCallback(int k, int s, int a, int m)
	{
		mainCamera->keyboardControlCallback(k,s,a,m);

		keyboardFunc(k,s,a,m);

		ImGui_ImplGlfwGL3_KeyCallback(window,k,s,a,m);
 	}

	inline void setupCallbackHelper(GLFWwindow* window, Camera* mainCam)
	{
	    ImGui_ImplGlfwGL3_Init(window, true);
		
		glfwGetCursorPos(window, &old_x, &old_y);
		
		mainCamera = mainCam;
		
		CallbackHelper::window = window;

		setCursorPosCallback(window, cursorPosCallback);
		setMouseButtonCallback(window, mouseButtonCallback);
		setKeyCallback(window, keyboardCallback);
	}
}
/***********************************************/

inline void updateLightCamera(Camera& mainCamera, Camera& lightSourceCamera)
{
	//TODO move lightSourceCamera according to mainCamera
}

#endif