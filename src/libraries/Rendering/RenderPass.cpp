#include "RenderPass.h"

#include "Rendering/OpenGLContext.h"

RenderPass::RenderPass(ShaderProgram* shaderProgram, FrameBufferObject* fbo)
{
	m_shaderProgram = shaderProgram;
	m_fbo = fbo;
	m_viewport = glm::vec4(-1.0f);
	
	p_perRenderableFunction = nullptr;

	m_clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	if (fbo)
	{
		m_viewport.x = 0;
		m_viewport.y = 0;
		
		m_viewport.z = fbo->getWidth();
		m_viewport.w = fbo->getHeight();
	}
}

void RenderPass::setClearColor(float r, float g, float b, float a)
{
	m_clearColor = glm::vec4(r,g,b,a);
}

RenderPass::~RenderPass()
{
	
}

void RenderPass::setShaderProgram(ShaderProgram* shaderProgram)
{
	m_shaderProgram = shaderProgram;
}

void RenderPass::setFrameBufferObject( FrameBufferObject* fbo)
{
	// set viewport if no viewport has been set before
	if ( m_viewport == glm::ivec4(-1) && fbo != 0)
	{
		m_viewport.x = 0;
		m_viewport.y = 0;
		m_viewport.z = fbo->getWidth();
		m_viewport.w = fbo->getHeight();
	}
	m_fbo = fbo;
}

FrameBufferObject* RenderPass::getFrameBufferObject()
{
	return m_fbo;
}

ShaderProgram* RenderPass::getShaderProgram()
{
	return m_shaderProgram;
}

void RenderPass::clearBits()
{
	for (unsigned int i = 0; i < m_clearBits.size(); i++)
	{
		if (m_clearBits[i] == GL_COLOR_BUFFER_BIT || m_clearBits[i] == GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
		{
			glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
		}
		glClear( m_clearBits[i] );
	}
}

void RenderPass::preRender()
{
}

void RenderPass::uploadUniforms()
{
	for(unsigned int i = 0; i < m_uniforms.size(); i++)
	{
		m_uniforms[i]->uploadUniform( m_shaderProgram );
	}
}

// static glm::vec4 temp_viewport;
void RenderPass::render()
{
	if (m_fbo)
	{
		OPENGLCONTEXT->bindFBO( m_fbo->getFramebufferHandle( ) );
	}
	else{
		OPENGLCONTEXT->bindFBO( 0);
	}

	m_shaderProgram->use();
	if (m_viewport != glm::ivec4(-1))
	{
		OPENGLCONTEXT->setViewport( (GLint) m_viewport.x, (GLint) m_viewport.y, (GLsizei) m_viewport.z, (GLsizei) m_viewport.w);
	}

	clearBits();

	enableStates();
	disableStates();

	preRender();
	for(unsigned int i = 0; i < m_renderables.size(); i++)
	{
		uploadUniforms();

		if (p_perRenderableFunction != nullptr)
		{
			(*p_perRenderableFunction)(m_renderables[i]);
		}

		m_renderables[i]->draw();
	}
	postRender();

	restoreStates();
	// if (m_viewport != glm::ivec4(-1))
	// {
	// 	OPENGLCONTEXT->setViewport( (GLint) temp_viewport.x, (GLint) temp_viewport.y, (GLsizei) temp_viewport.z, (GLsizei) temp_viewport.w);
	// }
}

// static glm::vec4 temp_viewport;
void RenderPass::renderInstanced(int numInstances)
{
	if (m_fbo){OPENGLCONTEXT->bindFBO(m_fbo->getFramebufferHandle( ) );}
	else{OPENGLCONTEXT->bindFBO(0); }

	m_shaderProgram->use();
	if (m_viewport != glm::ivec4(-1)){ OPENGLCONTEXT->setViewport( (GLint) m_viewport.x, (GLint) m_viewport.y, (GLsizei) m_viewport.z, (GLsizei) m_viewport.w); }

	clearBits();

	enableStates();
	disableStates();

	preRender();
	for(unsigned int i = 0; i < m_renderables.size(); i++)
	{
		uploadUniforms();
		if (p_perRenderableFunction != nullptr)
		{
			(*p_perRenderableFunction)(m_renderables[i]);
		}
		m_renderables[i]->drawInstanced( numInstances );
	}

	postRender();
	restoreStates();
}

void RenderPass::postRender()
{
}

void RenderPass::setPerRenderableFunction(std::function<void(Renderable*)>* perRenderableFunction)
{p_perRenderableFunction = perRenderableFunction;}

void RenderPass::addRenderable(Renderable* renderable)
{
	m_renderables.push_back(renderable);
}

std::vector< Renderable* > RenderPass::getRenderables()
{
	return m_renderables;
}

void RenderPass::setViewport(int x, int y, int width, int height)
{
	m_viewport.x = x;
	m_viewport.y = y;
	m_viewport.z = width;
	m_viewport.w = height;
}

void RenderPass::addClearBit(GLbitfield clearBit)
{
	m_clearBits.push_back(clearBit);
}

void RenderPass::addEnable(GLenum state)
{
	m_enable.push_back(state);
	m_enableTEMP.push_back( 0 );
}

void RenderPass::addDisable(GLenum state)
{
	m_disable.push_back(state);
	m_disableTEMP.push_back( 0 );
}

void RenderPass::enableStates()
{
	for (unsigned int i = 0; i < m_enable.size(); i++)
	{
		m_enableTEMP[i] = OPENGLCONTEXT->isEnabled(i);	
		OPENGLCONTEXT->setEnabled(m_enable[i], true);
	}
}

void RenderPass::disableStates()
{
	for (unsigned int i = 0; i < m_disable.size(); i++)
	{
		m_disableTEMP[i] = OPENGLCONTEXT->isEnabled(i);
		OPENGLCONTEXT->setEnabled(m_disable[i], false);
	}
}

void RenderPass::restoreStates()
{
	for (unsigned int i = 0; i < m_enableTEMP.size(); i++)
	{
		if ( !m_enableTEMP[i] )	// was not enabled but got enabled
		{
			OPENGLCONTEXT->setEnabled(m_enable[i],false);
		}
	}
	for (unsigned int i = 0; i < m_disableTEMP.size(); i++)
	{
		if ( m_disableTEMP[i] )	// was enabled but got disabled
		{
			OPENGLCONTEXT->setEnabled(m_disable[i], true);	
		}
	}
}

void RenderPass::addUniform(Uploadable* uniform) {
	m_uniforms.push_back( uniform );
}

void RenderPass::removeRenderable(Renderable* renderable) {
	for ( std::vector<Renderable* >::iterator it = m_renderables.begin(); it != m_renderables.end(); ++it)
	{
		if ( (*it) == renderable )
		{
			m_renderables.erase( it );
			return;
		}
	}
}

void RenderPass::clearRenderables() {
	m_renderables.clear();
}

void RenderPass::removeEnable(GLenum state) {
	for ( std::vector< GLenum >::iterator it = m_enable.begin(); it != m_enable.end(); ++it)
	{
		if ( (*it) == state )
		{
			m_enable.erase( it );
			return;
		}
	}
}

void RenderPass::removeDisable(GLenum state) {
	for ( std::vector< GLenum >::iterator it = m_disable.begin(); it != m_disable.end(); ++it)
	{
		if ( (*it) == state )
		{
			m_disable.erase( it );
			return;
		}
	}
}

void RenderPass::removeClearBit(GLbitfield clearBit) {
	for ( std::vector< GLbitfield >::iterator it = m_clearBits.begin(); it != m_clearBits.end(); ++it)
	{
		if ( (*it) == clearBit)
		{
			m_clearBits.erase( it );
			return;
		}
	}
}
