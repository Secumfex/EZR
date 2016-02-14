#ifndef GLTOOLS_H
#define GLTOOLS_H

#include "Core/DebugLog.h"
#include <Importing/Importer.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <glm/glm.hpp>

class FrameBufferObject;

GLFWwindow* generateWindow(int width = 1280, int height = 720, int posX = 100, int posY = 100); //!< initialize OpenGL (if not yet initialized) and create a GLFW window
bool shouldClose(GLFWwindow* window);
void swapBuffers(GLFWwindow* window);
void destroyWindow(GLFWwindow* window);
void render(GLFWwindow* window, std::function<void (double)> loop); //!< keep executing the provided loop function until the window is closed, swapping buffers and computing frame time (passed as argument to loop function)
GLenum checkGLError(bool printIfNoError = false); //!< check for OpenGL errors and also print it to the console (optionally even if no error occured)
std::string decodeGLError(GLenum error); //!< return string corresponding to an OpenGL error code (use with checkGLError)

void setKeyCallback(GLFWwindow* window, std::function<void (int, int, int, int)> func); //!< set callback function called when a key is pressed
void setMouseButtonCallback(GLFWwindow* window, std::function<void (int, int, int)> func); //!< set callback function called when a mouse button is pressed
void setCharCallback(GLFWwindow* window, std::function<void (unsigned int)> func); //!< set callback function called when a unicode character is put in
void setCursorPosCallback(GLFWwindow* window, std::function<void (double, double)> func); //!< set callback function called when cursor position changes
void setScrollCallback(GLFWwindow* window, std::function<void (double, double)> func); //!< set callback function called when scrolling
void setCursorEnterCallback(GLFWwindow* window, std::function<void (int)> func); //!< set callback function called when cursor enters window
void setWindowResizeCallback(GLFWwindow* window, std::function<void (int, int)> func); //!< set callback function called when cursor enters window

glm::vec2 getMainWindowResolution(); //!< returns width and height of the main window (if it exists)
glm::vec2 getResolution(GLFWwindow* window);
float getRatio(GLFWwindow* window); //!< returns (width / height) of the window

/**
* @brief copies the content of one frame buffer to another, either a color attachment or the depth buffer etc
* @param bitField defines the content to copy, e.g. GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT
* @param readBuffer (optional) can be set if bitField is GL_COLOR_BUFFER_BIT, if GL_NONE is provided, the default is used (GL_COLOR_ATTACHMENT0) or (GL_BACK), if source is 0 (window)
* @param filter (optional) can be set if bitField is GL_COLOR_BUFFER_BIT, if GL_NONE is provided, GL_NEAREST is used
*/
void copyFBOContent(FrameBufferObject* source, FrameBufferObject* target, GLbitfield bitField, GLenum readBuffer = GL_NONE, GLenum filter = GL_NONE , glm::vec2 defaultFBOSize = glm::vec2(-1.0f, -1.0f));

template <class T>
GLuint bufferData(const std::vector<T>& content, GLenum drawType = GL_STATIC_DRAW)
{
    GLuint vbo = 0;
	if ( content.size() != 0 )// && content.size() % dimensions == 0 )
	{
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, content.size() * sizeof(T), &content[0], drawType);
	}
    return vbo;
}

/** upload the provided volume data to a 3D OpenGL texture object, i.e. CT-Data*/
template <typename T>
GLuint loadTo3DTexture(VolumeData<T>& volumeData, GLenum internalFormat = GL_R16I, GLenum format = GL_RED_INTEGER, GLenum type = GL_SHORT)
{
	GLuint volumeTexture;

	glEnable(GL_TEXTURE_3D);
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &volumeTexture);
	glBindTexture(GL_TEXTURE_3D, volumeTexture);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);

	// allocate GPU memory
	glTexStorage3D(GL_TEXTURE_3D
		, 1
		, internalFormat
		, volumeData.size_x
		, volumeData.size_y
		, volumeData.size_z
	);

	// upload data
	glTexSubImage3D(GL_TEXTURE_3D
		, 0
		, 0
		, 0
		, 0
		, volumeData.size_x
		, volumeData.size_y
		, volumeData.size_z
		, format
		, type
		, &(volumeData.data[0])
	);

	return volumeTexture;
}

#endif