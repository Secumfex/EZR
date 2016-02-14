#include "GLTools.h"

static bool g_initialized = false;
static glm::vec2 g_mainWindowSize = glm::vec2(0,0);

GLFWwindow* generateWindow(int width, int height, int posX, int posY) {
	if (g_initialized == false)
	{
		glfwInit();
		#ifdef __APPLE__
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		glewExperimental = GL_TRUE;
		#endif
	}
	
	GLFWwindow* window = glfwCreateWindow(width, height, "OpenGL Window", NULL, NULL);
	glfwSetWindowPos(window, posX, posY);
	glfwSetWindowSize(window, width, height);
	glfwMakeContextCurrent(window);

	glViewport(0,0,width,height);

	if (g_initialized == false)
	{
		glewInit();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);
		
		g_mainWindowSize = glm::vec2(width,height);

		g_initialized = true;
	}

//	registerDefaultGLFWCallbacks(window);

	return window;
}

glm::vec2 getMainWindowResolution()
{
	return g_mainWindowSize;
}


bool shouldClose(GLFWwindow* window)
{
	return (glfwWindowShouldClose(window) != 0);
}

void swapBuffers(GLFWwindow* window)
{
	glfwSwapBuffers(window);
    glfwPollEvents();
}

void destroyWindow(GLFWwindow* window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void render(GLFWwindow* window, std::function<void (double)> loop) {
	float lastTime = 0.0;
	while ( !glfwWindowShouldClose(window)) {
		float currentTime =static_cast<float>(glfwGetTime());
		loop(currentTime - lastTime);
		lastTime = currentTime;

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

GLenum checkGLError(bool printIfNoError)
{
	GLenum error = glGetError();
	
	switch (error) 
	{
	case GL_INVALID_VALUE:
		DEBUGLOG->log("GL_INVALID_VALUE");
		break;
	case GL_INVALID_ENUM:
		DEBUGLOG->log("GL_INVALID_ENUM");
		break;
	case GL_INVALID_OPERATION:
		DEBUGLOG->log("GL_INVALID_OPERATION");
		break;
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		DEBUGLOG->log("GL_INVALID_FRAMEBUFFER_OPERATION");
		break;
	case GL_OUT_OF_MEMORY:
		DEBUGLOG->log("GL_OUT_OF_MEMORY");
		break;		
	case GL_NO_ERROR:				
		if (printIfNoError)
		{
			DEBUGLOG->log("GL_NO_ERROR"); // do not use error stream for this
		}
		break;
	}
	return error;
}


std::string decodeGLError(GLenum error)
{
	switch (error) 
	{
	case GL_INVALID_VALUE:
		return std::string("GL_INVALID_VALUE");
	case GL_INVALID_ENUM:
		return std::string("GL_INVALID_ENUM");
	case GL_INVALID_OPERATION:
		return std::string("GL_INVALID_OPERATION");
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return std::string("GL_INVALID_FRAMEBUFFER_OPERATION");
	case GL_OUT_OF_MEMORY:
		return std::string("GL_OUT_OF_MEMORY");
	case GL_NO_ERROR:
		return std::string("GL_NO_ERROR");
	}

	return std::string(); // not a valid error state
}


void setKeyCallback(GLFWwindow* window, std::function<void (int, int, int, int)> func) {
	static std::function<void (int, int, int, int)> func_bounce = func;
	glfwSetKeyCallback(window, [] (GLFWwindow* w, int k, int s, int a, int m) {
		func_bounce(k, s, a, m);
	});
}

void setMouseButtonCallback(GLFWwindow* window, std::function<void (int, int, int)> func) {
	static std::function<void (int, int, int)> func_bounce = func;
	glfwSetMouseButtonCallback(window, [] (GLFWwindow* w, int b, int a, int m) {
		func_bounce(b, a, m);
	});
}

void setCharCallback(GLFWwindow* window, std::function<void (unsigned int)> func) {
	static std::function<void (unsigned int)> func_bounce = func;
	glfwSetCharCallback(window, [] (GLFWwindow* w, unsigned int c) {
		func_bounce(c);
	});
}

void setCursorPosCallback(GLFWwindow* window, std::function<void (double, double)> func) {
	static std::function<void (double, double)> func_bounce = func;
	glfwSetCursorPosCallback(window, [] (GLFWwindow* w, double x, double y) {
		func_bounce(x, y);
	});
}

void setScrollCallback(GLFWwindow* window, std::function<void (double, double)> func) {
	static std::function<void (double, double)> func_bounce = func;
	glfwSetScrollCallback(window, [] (GLFWwindow* w, double x, double y) {
		func_bounce(x, y);
	});
}

void setCursorEnterCallback(GLFWwindow* window, std::function<void (int)> func) {
	static std::function<void (int)> func_bounce = func;
	glfwSetCursorEnterCallback(window, [] (GLFWwindow* w, int e) {
		func_bounce(e);
	});
}

void setWindowResizeCallback(GLFWwindow* window, std::function<void (int,int)> func) {
	static std::function<void (int,int)> func_bounce = func;
	glfwSetWindowSizeCallback(window, [] (GLFWwindow* w, int wid, int hei) {
		func_bounce(wid, hei);
	});
}

glm::vec2 getResolution(GLFWwindow* window) {
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    return glm::vec2(float(w), float(h));
}

float getRatio(GLFWwindow* window) {
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    return float(w)/float(h);
}


#include <Rendering/FrameBufferObject.h>
void copyFBOContent(FrameBufferObject* source, FrameBufferObject* target, GLbitfield bitField, GLenum readBuffer, GLenum filter, glm::vec2 defaultFBOSize )
{
	// bind framebuffers
	glBindFramebuffer(GL_READ_FRAMEBUFFER, (source != nullptr) ? source->getFramebufferHandle() : 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (target != nullptr) ? target->getFramebufferHandle() : 0);
	
	GLint defaultFBOWidth =  (GLint) (defaultFBOSize.x != -1.0f) ? defaultFBOSize.x : g_mainWindowSize.x;
	GLint defaultFBOHeight = (GLint) (defaultFBOSize.x != -1.0f) ? defaultFBOSize.x : g_mainWindowSize.x;
	
	// color buffer is to be copied
	if (bitField == GL_COLOR_BUFFER_BIT)
	{
		// default
		if (readBuffer == GL_NONE)
		{
			glReadBuffer( (source == nullptr) ? GL_BACK : GL_COLOR_ATTACHMENT0);
		}
		else // set manually
		{
			glReadBuffer( readBuffer );
		}
		// copy content

		glBlitFramebuffer(
			0,0,(source != nullptr) ? source->getWidth() : defaultFBOWidth, (source!=nullptr)   ? source->getHeight() : defaultFBOHeight,
			0,0,(target != nullptr) ? target->getWidth() : defaultFBOWidth, (target != nullptr) ? target->getHeight() : defaultFBOHeight,
			bitField, (filter == GL_NONE) ? GL_NEAREST : filter);
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return;
	}
	if (bitField == GL_DEPTH_BUFFER_BIT)
	{
		glBlitFramebuffer(
			0,0,(source != nullptr) ? source->getWidth() : defaultFBOWidth, (source!=nullptr)   ? source->getHeight() : defaultFBOHeight, 
			0,0,(target != nullptr) ? target->getWidth() : defaultFBOWidth, (target != nullptr) ? target->getHeight() : defaultFBOHeight,
			bitField, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return;
	}

	// custom
	glBlitFramebuffer(
		0,0,(source != nullptr) ? source->getWidth() : defaultFBOWidth, (source!=nullptr)   ? source->getHeight() : defaultFBOHeight, 
		0,0,(target != nullptr) ? target->getWidth() : defaultFBOWidth, (target != nullptr) ? target->getHeight() : defaultFBOHeight,
		bitField, filter);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
