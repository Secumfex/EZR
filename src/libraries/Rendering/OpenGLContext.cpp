#include <Rendering/OpenGLContext.h>

#include <glm/gtc/type_ptr.hpp>

OpenGLContext::OpenGLContext()
{
	clearCache();
}

OpenGLContext::~OpenGLContext()
{

}

void OpenGLContext::clearCache()
{
	cacheTextures.clear();
	cacheVAO = -1;
	cacheFBO = -1;
	cacheShader = -1;

	cacheViewport = glm::ivec4(-1);
	cacheWindowSize = glm::ivec2(-1);
}

void OpenGLContext::updateCache()
{
	updateBindingCache();
	updateWindowCache();
	updateTextureCache();
}

void OpenGLContext::updateBindingCache()
{
	// current framebuffer
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*) &cacheFBO);

	// current vao
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*) &cacheVAO);

	// current shader
	glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &cacheShader);
}

void OpenGLContext::updateWindowCache()
{
	glGetIntegerv(GL_VIEWPORT, (GLint*) glm::value_ptr(cacheViewport));
	
	cacheWindow = glfwGetCurrentContext();
	
	glfwGetWindowSize(cacheWindow, &cacheWindowSize[0],&cacheWindowSize[1]);
}

void OpenGLContext::updateTextureCache(int MAX_NUM_CACHED_TEXTURES)
{
	// yuck
	for (int i = 0; i < MAX_NUM_CACHED_TEXTURES; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		GLint textureHandle;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureHandle);
		if ( textureHandle == 0 )
		{
			glGetIntegerv(GL_TEXTURE_BINDING_3D, &textureHandle);
			if ( textureHandle == 0 )
			{
				glGetIntegerv(GL_TEXTURE_BINDING_1D, &textureHandle);
				if ( textureHandle == 0 )
					glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &textureHandle);
			}
		}

		bool hasTexture = (cacheTextures.find(i) != cacheTextures.end());
		if ( (!hasTexture && textureHandle != 0) || (hasTexture && textureHandle != cacheTextures.at(i)))
		{
			cacheTextures[i] = textureHandle;
		}
	}
}

void OpenGLContext::updateCurrentTextureCache()
{
	// only texture units already in cache
	for ( auto e : cacheTextures)
	{
		glActiveTexture(GL_TEXTURE0 + e.first);
		
		GLint textureHandle;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureHandle);
		if ( textureHandle == 0 )
		{
			glGetIntegerv(GL_TEXTURE_BINDING_3D, &textureHandle);
			if ( textureHandle == 0 )
			{
				glGetIntegerv(GL_TEXTURE_BINDING_1D, &textureHandle);
				if ( textureHandle == 0 )
					glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &textureHandle);
			}
		}

		// update with textureHandle, regardless of it being 0
		cacheTextures.at(e.first) = textureHandle;
	}
}

void OpenGLContext:: bindVAO(GLuint vao)
{
	if ( cacheVAO != vao )
	{
		glBindVertexArray(vao);
		cacheVAO = vao;
	}
}

void OpenGLContext::useShader(GLuint shaderProgram)
{
	if (cacheShader != shaderProgram )
	{
		glUseProgram(shaderProgram);
		cacheShader = shaderProgram;
	}
}

void OpenGLContext::bindTexture(GLuint texture, GLenum type)
{
	GLint unit;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &unit);
	bindTexture(texture, unit, type);
}

void OpenGLContext::bindTexture(GLuint texture, GLint unit, GLenum type)
{
	if ( cacheTextures.find(unit) != cacheTextures.end() || cacheTextures.at(unit) != texture)
	{
		glBindTexture(type, texture);
		cacheTextures[unit] = texture;
	}
}

void OpenGLContext::setViewport(int x, int y, int width, int height)
{
	setViewport(glm::ivec4(x,y,width,height));
}
void OpenGLContext::setViewport(glm::vec4 viewport)
{
	setViewport(glm::ivec4(viewport));
}
void OpenGLContext::setViewport(glm::ivec4 viewport)
{
	if (cacheViewport != viewport )
	{
		glViewport( (GLint) viewport.x, (GLint) viewport.y, (GLsizei) viewport.z, (GLsizei) viewport.w);
		cacheViewport = viewport;
	}
}

void OpenGLContext::setWindowSize(GLFWwindow* window, int width, int height)
{
	setWindowSize(window, glm::ivec2(width,height));
}
void OpenGLContext::setWindowSize(GLFWwindow* window, glm::vec2 windowSize)
{
	setWindowSize(window, glm::ivec2(windowSize));
}
void OpenGLContext::setWindowSize(GLFWwindow* window, glm::ivec2 windowSize)
{
	if (cacheWindowSize != windowSize )
	{
		glfwSetWindowSize(window, windowSize.x, windowSize.y);
		cacheWindowSize = windowSize;
	}
}
