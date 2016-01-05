#ifndef FRAMEBUFFEROBJECT_H
#define FRAMEBUFFEROBJECT_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <vector>
#include <string>

class FrameBufferObject
{
protected:
	GLuint m_frameBufferHandle;

	int m_width;
	int m_height;

	int m_numColorAttachments;

	GLuint m_depthTextureHandle;

	std::map< GLenum, GLuint > m_colorAttachments;
	std::map<std::string, GLuint> m_textureMap;
	std::vector<GLenum > m_drawBuffers;
public:

	static GLenum s_internalFormat; //!< used as parameter to allocate color attachment texture memory
	static GLenum s_format; //!< used as parameter to allocate color attachment texture memory using glTexImage2D
	static bool s_useTexStorage2D; //!< describes whether glTexStorage2D is used in favor of glTexImage2D (default: false)
	static GLenum s_type; //!< used as parameter to allocate color attachment texture memory using glTexImage2D

	FrameBufferObject(int width = 800, int height = 600); //!< creates a fbo containing a depth buffer but no color attachments

	/** @brief creates a fbo containing a depth buffer and as many color attachments as there are entries in the 'outputMap'
	* @details the outputmap should contain pairs of an output name and layout location of fragment shader outputs.
	* Using this, the corresponding texture handles may be retrieved using getBuffer() in addition to getColorAttachmentTextureHandle().
	*/
	FrameBufferObject(std::map<std::string, int>* outputMap, int width, int height, GLint internalFormat = GL_RGBA);
	~FrameBufferObject();

	void createDepthTexture();

	GLuint createFramebufferTexture();
	void addColorAttachments(int amount); //!< adds the provided amount of color attachment textures to the fbo, should correspond to the amount of outputs of the shader writing to this fbo

	GLuint getColorAttachmentTextureHandle(GLenum attachment); //!< retrieve the texture handle of the provided color attachment, e.g. GL_COLOR_ATTACHMENT0, if existing (else: 0)
	void setColorAttachmentTextureHandle(GLenum attachment, GLuint textureHandle); //!< may be used to share a texture between multiple fbos (of equal size) (untested)

	GLuint getFramebufferHandle();

	int getWidth();
	int getHeight();

	const std::map<GLenum, GLuint>& getColorAttachments() const;
	void setColorAttachments(const std::map<GLenum, GLuint>& colorAttachments);
	GLuint getDepthTextureHandle() const;
	void setDepthTextureHandle(GLuint depthTextureHandle);
	const std::vector<GLenum>& getDrawBuffers() const;
	void setDrawBuffers(const std::vector<GLenum>& drawBuffers);
	void setFramebufferHandle(GLuint framebufferHandle);
	void setHeight(int height);
	int getNumColorAttachments() const;
	void setNumColorAttachments(int numColorAttachments);
	void setWidth(int width);

	void bind(); //!< binds the fbo and sets the viewport to its size
	void unbind(); //!< unbinds the fbo (binds screen, i.e. 0)

	GLuint getBuffer(std::string name); //!< Get the texture handle corresponding to a certain buffer name.

	void setFrameBufferObject(const GLuint& frameBufferObjectHandle, const int& width, const int& height, const std::map<std::string, GLuint>& textureMap, GLuint depthTexture);
};

#endif