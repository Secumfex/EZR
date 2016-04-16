#include "Rendering/FrameBufferObject.h"

#include "Core/DebugLog.h"
#include "Rendering/OpenGLContext.h"

GLenum FrameBufferObject::s_internalFormat  = GL_RGBA;	// default
GLenum FrameBufferObject::s_format 			= GL_RGBA;	// default
GLenum FrameBufferObject::s_type 			= GL_UNSIGNED_BYTE;	// default
bool FrameBufferObject::s_useTexStorage2D	= false;	// default

FrameBufferObject::FrameBufferObject(int width, int height)
{
	glGenFramebuffers(1, &m_frameBufferHandle);
	OPENGLCONTEXT->bindFBO(m_frameBufferHandle);

	m_width = width;
	m_height = height;

	createDepthTexture();

	m_numColorAttachments = 0;
}

void FrameBufferObject::createDepthTexture()
{
	OPENGLCONTEXT->bindFBO(m_frameBufferHandle);

	glGenTextures(1, &m_depthTextureHandle);
	OPENGLCONTEXT->bindTexture(m_depthTextureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTextureHandle, 0);

	OPENGLCONTEXT->bindFBO(0);
}

GLuint FrameBufferObject::createFramebufferTexture()
{
	GLuint textureHandle;
	glGenTextures(1, &textureHandle);
	OPENGLCONTEXT->bindTexture(textureHandle);

	if ( s_useTexStorage2D )
	{
		// for testing purposes
		glTexStorage2D(GL_TEXTURE_2D, 1, s_internalFormat, m_width, m_height);	
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, s_internalFormat, m_width, m_height, 0, s_format, s_type, 0);	
	}
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	OPENGLCONTEXT->bindTexture(0);
	return textureHandle;
}

void FrameBufferObject::addColorAttachments(int amount)
{
	int maxColorAttachments;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
	if ( m_numColorAttachments + amount <=  maxColorAttachments)
	{
//		DEBUGLOG->log("max color attachments: ", maxColorAttachments);
		OPENGLCONTEXT->bindFBO(m_frameBufferHandle);

		DEBUGLOG->log("Creating Color Attachments: ", amount);
		DEBUGLOG->indent();
		for (int i = 0; i < amount; i ++)
		{
			GLuint textureHandle = createFramebufferTexture();
			
			OPENGLCONTEXT->bindTexture(textureHandle);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + m_numColorAttachments + i, GL_TEXTURE_2D, textureHandle, 0);
			OPENGLCONTEXT->bindTexture(0);
			
			m_colorAttachments[GL_COLOR_ATTACHMENT0 + m_numColorAttachments + i] = textureHandle;
			m_drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + m_numColorAttachments + i);
		}
		DEBUGLOG->outdent();

		m_numColorAttachments = m_colorAttachments.size();
		glDrawBuffers(m_drawBuffers.size(), &m_drawBuffers[0]);
		OPENGLCONTEXT->bindFBO(0);
	}
}

void FrameBufferObject::setColorAttachmentTextureHandle( GLenum attachment, GLuint textureHandle )
{
	OPENGLCONTEXT->bindTexture(textureHandle);
	int width, height;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	if (  width != m_width || height != m_height )
	{
		DEBUGLOG->log("ERROR : size of texture differs from frame buffer size");
		return;
	}
	if ( m_colorAttachments.find( attachment ) != m_colorAttachments.end() )
	{
		GLuint oldAttachment = m_colorAttachments[ attachment ];
		DEBUGLOG->log("WARNING : remember to delete the old texture handle", oldAttachment);

		m_colorAttachments[ attachment ] = textureHandle;
		OPENGLCONTEXT->bindFBO(m_frameBufferHandle);
		
		OPENGLCONTEXT->bindTexture(textureHandle);
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, textureHandle, 0);
		OPENGLCONTEXT->bindTexture(0);
	}
	else
	{
		DEBUGLOG->log("ERROR : specified color attachment does not exist");
	}
	OPENGLCONTEXT->bindFBO(0);
}

GLuint FrameBufferObject::getColorAttachmentTextureHandle(GLenum attachment)
{
	if ( m_colorAttachments.find(attachment) != m_colorAttachments.end())
	{
		return m_colorAttachments[attachment];
	}
	else{
		DEBUGLOG->log("ERROR: couldn't find color attachment");
		return 0;
	}
}

GLuint FrameBufferObject::getFramebufferHandle()
{
	return m_frameBufferHandle;
}

int FrameBufferObject::getWidth()
{
	return m_width;
}

const std::map<GLenum, GLuint>& FrameBufferObject::getColorAttachments() const {
	return m_colorAttachments;
}

void FrameBufferObject::setColorAttachments(
		const std::map<GLenum, GLuint>& colorAttachments) {
	m_colorAttachments = colorAttachments;
}

GLuint FrameBufferObject::getDepthTextureHandle() const {
	return m_depthTextureHandle;
}

void FrameBufferObject::setDepthTextureHandle(GLuint depthTextureHandle) {
	m_depthTextureHandle = depthTextureHandle;
	OPENGLCONTEXT->bindFBO(m_frameBufferHandle);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTextureHandle, 0);
	OPENGLCONTEXT->bindFBO(0);
}

const std::vector<GLenum>& FrameBufferObject::getDrawBuffers() const {
	return m_drawBuffers;
}

void FrameBufferObject::setDrawBuffers(const std::vector<GLenum>& drawBuffers) {
	m_drawBuffers = drawBuffers;
}

void FrameBufferObject::setFramebufferHandle(GLuint framebufferHandle) {
	m_frameBufferHandle = framebufferHandle;
}

void FrameBufferObject::setHeight(int height) {
	m_height = height;
}

int FrameBufferObject::getNumColorAttachments() const {
	return m_numColorAttachments;
}

void FrameBufferObject::setNumColorAttachments(int numColorAttachments) {
	m_numColorAttachments = numColorAttachments;
}

FrameBufferObject::~FrameBufferObject() {
	// TODO free OpenGL textures etc.
}

void FrameBufferObject::setWidth(int width) {
	m_width = width;
}

int FrameBufferObject::getHeight()
{
	return m_height;
}

FrameBufferObject::FrameBufferObject(std::map<std::string, ShaderProgram::Info>* outputMap, int width, int height) 
	: m_width(width), m_height(height)
{
	//Generate FBO
	glGenFramebuffers(1, &m_frameBufferHandle);
    OPENGLCONTEXT->bindFBO(m_frameBufferHandle);

    //Generate color textures
    int size = outputMap->size();

	// find the biggest user defined output location
	for (auto e : *outputMap)
	{
		if (e.second.location+1 > size)
		{
			size = e.second.location+1;
		}
	}

    std::vector<GLuint> drawBufferHandles(size, GL_NONE);

	
	OPENGLCONTEXT->activeTexture(GL_TEXTURE0);
	int i = 0;
    for (auto e : *outputMap) 
    {	
		GLuint handle = createFramebufferTexture();

		GLuint currentAttachment = GL_COLOR_ATTACHMENT0 + static_cast<unsigned int>(e.second.location); //e.second;
		//GLuint currentAttachment = GL_COLOR_ATTACHMENT0 + i; //e.second;
		//DEBUGLOG->log("buffer    : " + e.first);
		//DEBUGLOG->log("tex handle: ", handle);
		//DEBUGLOG->log("attachment: ", currentAttachment - GL_COLOR_ATTACHMENT0);
		//DEBUGLOG->log("output loc: ", e.second.location);
		//DEBUGLOG->log("------------------------");

	    glFramebufferTexture2D(GL_FRAMEBUFFER, currentAttachment, GL_TEXTURE_2D, handle, 0);

    	m_textureMap[e.first] = handle;

		/** according to OpenGL glDrawBuffers specification:
		* If a fragment shader writes a value to one or more user defined output variables, 
		* then the value of each variable will be written into the buffer specified at a location 
		* within bufs corresponding to the location assigned to that user defined output. 
		* The draw buffer used for user defined outputs assigned to locations greater than 
		* or equal to n is implicitly set to GL_NONE and any data written to such an output is discarded
		*/
		if ( e.second.location < drawBufferHandles.size() )
		{
			drawBufferHandles[ e.second.location ] = currentAttachment;
		}
		else
		{
			DEBUGLOG->log("ERROR: output location was larger than allocated amount of draw buffers!?");
		}

	    //drawBufferHandles[i] = currentAttachment;
	    m_colorAttachments[currentAttachment] = handle;
	    i++;
    }

	if (!drawBufferHandles.empty() )
    {
		glDrawBuffers(size, &drawBufferHandles[0]);
	}

	//glGenTextures( 1, &m_depthTextureHandle);
	//
	// OPENGLCONTEXT->bindTexture(m_depthTextureHandle);
	//glTexImage2D(GL_TEXTURE_2D, 0,GL_DEPTH_COMPONENT24, m_width, m_height, 0,GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTextureHandle, 0);

	createDepthTexture();

	// Any errors while generating fbo ?
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		DEBUGLOG->log("ERROR: Unable to create FBO!");
	}

	OPENGLCONTEXT->bindFBO(0);	
}

void FrameBufferObject::bind() {
	OPENGLCONTEXT->bindFBO(m_frameBufferHandle);
	glViewport( 0, 0, m_width, m_height);
}

void FrameBufferObject::unbind() {
	OPENGLCONTEXT->bindFBO(0);
}


void FrameBufferObject::mapColorAttachmentToBufferName(GLenum colorAttachment, std::string bufferName)
{
	GLuint handle = getColorAttachmentTextureHandle(colorAttachment);
	if ( handle != 0)
	{
		m_textureMap[bufferName] = handle;
	}
}

GLuint FrameBufferObject::getBuffer(std::string name) {
	if (m_textureMap.size() == 0)
	{
		DEBUGLOG->log("ERROR: buffer map has not been associated with names or fbo is empty");
		return 0;
	}
	if (m_textureMap.find(name) != m_textureMap.end())
	{
		return m_textureMap[name]; // safe access to map
	}
	return 0; // buffer does not exist
}

void FrameBufferObject::setFrameBufferObject( const GLuint& frameBufferHandle, const int& width, const int& height, const std::map<std::string, GLuint>& textureMap, GLuint depthTexture ) 
{
	this->m_frameBufferHandle = frameBufferHandle;
	m_width = width;
	m_height = height;
	m_textureMap = textureMap;
	m_depthTextureHandle = depthTexture;
}