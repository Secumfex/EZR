#include "Rendering/PostProcessing.h"

#include <Rendering/FrameBufferObject.h>
#include <Rendering/VertexArrayObjects.h>

namespace{float log_2( float n )  
{  
    return log( n ) / log( 2 );      // log(n)/log(2) is log_2. 
}}

PostProcessing::BoxBlur::BoxBlur(int width, int height, Quad* quad)
	: m_pushShaderProgram("/screenSpace/fullscreen.vert", "/screenSpace/pushBoxBlur.frag" )
	, m_height(height)
	, m_width(width)
{
	if (quad == nullptr){
		m_quad = new Quad();
		ownQuad = true;
	}else{
		m_quad = quad;
		ownQuad = false;
	}

	glGenTextures(1, &m_mipmapTextureHandle);
	glBindTexture(GL_TEXTURE_2D, m_mipmapTextureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width,height, 0, GL_RGBA, GL_UNSIGNED_INT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // does this do anything?

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); // does this do anything?
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT); 

	glGenerateMipmap(GL_TEXTURE_2D);
	int mipmapNumber = (int) log_2( max(width,height) );

	m_mipmapFBOHandles.resize(mipmapNumber);
	glGenFramebuffers(mipmapNumber, &m_mipmapFBOHandles[0]);

	for ( int i = 0; i < mipmapNumber; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_mipmapFBOHandles[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_mipmapTextureHandle, i);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_pushShaderProgram.bindTextureOnUse("tex", m_mipmapTextureHandle);
}

void PostProcessing::BoxBlur::pull()
{
	glBindTexture(GL_TEXTURE_2D, m_mipmapTextureHandle);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void PostProcessing::BoxBlur::push(int numLevels, int beginLevel)
{
	// check boundaries
	GLboolean depthTestEnableState = glIsEnabled(GL_DEPTH_TEST);
	if (depthTestEnableState) {glDisable(GL_DEPTH_TEST);}
	m_pushShaderProgram.use();
	for (int level = min( (int) m_mipmapFBOHandles.size()-2, beginLevel+numLevels-1); level >= beginLevel; level--)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_mipmapFBOHandles[level]);
		m_pushShaderProgram.update("level", level);
		m_quad->draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (depthTestEnableState){glEnable(GL_DEPTH_TEST);}
}

PostProcessing::BoxBlur::~BoxBlur()
{
	if (ownQuad) {delete m_quad;}
}

PostProcessing::DepthOfField::DepthOfField(int width, int height, Quad* quad)
	: m_calcCoCShader("/screenSpace/fullscreen.vert", "/screenSpace/postProcessCircleOfConfusion.frag")
	, m_dofShader("/screenSpace/fullscreen.vert", "/screenSpace/postProcessDOF.frag")
	, m_dofCompShader("/screenSpace/fullscreen.vert", "/screenSpace/postProcessDOFCompositing.frag")
	, m_width(width)
	, m_height(height)
{
	if (quad == nullptr){
		m_quad = new Quad();
		ownQuad = true;
	}else{
		m_quad = quad;
		ownQuad = false;
	}

	FrameBufferObject::s_internalFormat = GL_RGBA32F;
	m_cocFBO 	 = new FrameBufferObject(m_calcCoCShader.getOutputInfoMap(), width, height);
	m_hDofFBO 	 = new FrameBufferObject(m_dofShader.getOutputInfoMap(), width / 4.0, height );
	m_vDofFBO 	 = new FrameBufferObject(m_dofShader.getOutputInfoMap(), width / 4.0, height / 4.0);
	FrameBufferObject::s_internalFormat = GL_RGBA;
	m_dofCompFBO = new FrameBufferObject(m_dofCompShader.getOutputInfoMap(), width, height );
	
	for ( auto t : m_vDofFBO->getColorAttachments() )
	{
		glBindTexture(GL_TEXTURE_2D, t.second);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	
	m_dofCompShader.bindTextureOnUse("sharpFocusField", m_cocFBO->getBuffer("fragmentColor"));
	m_dofCompShader.bindTextureOnUse("blurryNearField", m_vDofFBO->getBuffer("nearResult"));
	m_dofCompShader.bindTextureOnUse("blurryFarField" , m_vDofFBO->getBuffer("blurResult"));

	// default settings
	m_calcCoCShader.update("focusPlaneDepths", glm::vec4(2.0,4.0,7.0,10.0));
	m_calcCoCShader.update("focusPlaneRadi",   glm::vec2(10.0f, -5.0f) );

	m_dofShader.update("maxCoCRadiusPixels", 10);
	m_dofShader.update("nearBlurRadiusPixels", 10);
	m_dofShader.update("invNearBlurRadiusPixels", 0.1f);

	m_dofCompShader.update("maxCoCRadiusPixels", 10.0f);
	m_dofCompShader.update("farRadiusRescale" , 2.0f);
}

PostProcessing::DepthOfField::~DepthOfField()
{
	if (ownQuad) {delete m_quad;}
}

void PostProcessing::DepthOfField::execute(GLuint positionMap, GLuint colorMap)
{
	// setup
	GLboolean depthTestEnableState = glIsEnabled(GL_DEPTH_TEST);
	if (depthTestEnableState) {glDisable(GL_DEPTH_TEST);}

	// compute COC map
	glViewport(0,0,m_width, m_height);
	m_cocFBO->bind();
	m_calcCoCShader.updateAndBindTexture("colorMap", 0, colorMap);
	m_calcCoCShader.updateAndBindTexture("positionMap", 1, positionMap);
	m_calcCoCShader.use();
	m_quad->draw();

	// compute DoF
	// horizontal pass 
	glViewport(0, 0, m_hDofFBO->getWidth(), m_hDofFBO->getHeight());
	m_hDofFBO->bind();
	m_dofShader.use();
	m_dofShader.update("HORIZONTAL", true);
	m_dofShader.updateAndBindTexture("blurSourceBuffer", 1, m_cocFBO->getBuffer("fragmentColor"));
	m_quad->draw();

	// vertical pass
	glViewport(0,0,m_vDofFBO->getWidth(), m_vDofFBO->getHeight());
	m_vDofFBO->bind();
	m_dofShader.update("HORIZONTAL", false);
	m_dofShader.updateAndBindTexture("blurSourceBuffer", 1, m_hDofFBO->getBuffer("blurResult"));
	m_dofShader.updateAndBindTexture("nearSourceBuffer", 2, m_hDofFBO->getBuffer("nearResult"));
	m_quad->draw();

	m_dofCompFBO->bind();
	m_dofCompShader.use();
	m_quad->draw();
} 
