#include "Rendering/PostProcessing.h"

#include <Rendering/FrameBufferObject.h>
#include <Rendering/VertexArrayObjects.h>

namespace{float log_2( float n )  
{  
    return log( n ) / log( 2 );      // log(n)/log(2) is log_2. 
}}

PostProcessing::BoxBlur::BoxBlur(int width, int height, Quad* quad)
	: m_pushShaderProgram("/screenSpace/fullscreen.vert", "/screenSpace/pushBoxBlur.frag" )
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

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST); 
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
