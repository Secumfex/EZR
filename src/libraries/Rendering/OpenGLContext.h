#ifndef OPENGLCONTEXT_H
#define OPENGLCONTEXT_H

#include <string>
#include <map>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <Core/Singleton.h>

/**
* @brief a convenience class that caches the values of OpenGL
*/
class OpenGLContext : public Singleton< OpenGLContext >
{
friend class Singleton< OpenGLContext >;
private:
	OpenGLContext();
	~OpenGLContext();
public:
	//std::map<GLint, GLint> cacheInt;
	//std::map<GLint, GLfloat> cacheFloat;
	std::map<GLint, GLuint> cacheTextures; // currently bound textures to texture units GL_TEXTURE0..31
	GLuint cacheVAO; // currently bound VAO
	GLuint cacheReadFBO; // currently bound FBO
	GLuint cacheDrawFBO; // currently bound FBO
	GLuint cacheShader; // currently active shader program
	GLenum cacheActiveTexture; // currently active texture unit

	GLFWwindow* cacheWindow;
	glm::ivec4 cacheViewport;
	glm::ivec2 cacheWindowSize;

	void clearCache();
	void updateCache(); // retrieve most commonly used OpenGL values and add to cache
	void updateBindingCache(); //retrieve currently bound FBO, VAO, ShaderProgram
	void updateWindowCache(); //retrieve current Viewport, WindowSize, Window
	void updateTextureCache(int MAX_NUM_CACHED_TEXTURES = 12); //retrieve currently bound textures
	void updateCurrentTextureCache(); //retrieve currently bound textures looking only at the units already in cache

	// methods only issuing OpenGL commands if cached value difffers
	void bindVAO(GLuint vao);
	void bindFBO(GLuint fbo, GLenum type = GL_FRAMEBUFFER);
	void useShader(GLuint shaderProgram);
	void activeTexture(GLenum unit);
	void bindTexture(GLuint texture, GLenum type = GL_TEXTURE_2D);
	void bindTextureToUnit(GLuint texture, GLenum unit, GLenum type = GL_TEXTURE_2D);
	void setViewport(int x, int y, int width, int height);
	void setViewport(const glm::vec4& viewport);
	void setViewport(const glm::ivec4& viewport);
	void setWindowSize(GLFWwindow* window, int width, int height);
	void setWindowSize(GLFWwindow* window, glm::vec2 windowSize);
	void setWindowSize(GLFWwindow* window, glm::ivec2 windowSize);
};

// for convenient access
#define OPENGLCONTEXT OpenGLContext::getInstance()

#endif
