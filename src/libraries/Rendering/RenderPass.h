#ifndef RENDERPASS_H
#define RENDERPASS_H

#include "Rendering/VertexArrayObjects.h"
#include "Rendering/FrameBufferObject.h"
#include "Rendering/ShaderProgram.h"
#include "Rendering/Uniform.h"

#include <vector>
#include <functional>

// template<typename T>
// struct vecUniform : public std::vector< Uniform<T>* > {};

class RenderPass{
protected:
	glm::ivec4 m_viewport;
	glm::vec4 m_clearColor;
	
	FrameBufferObject* m_fbo;
	ShaderProgram* m_shaderProgram;

	std::vector< Renderable* > m_renderables;

	std::vector< GLbitfield > m_clearBits;
	std::vector< GLenum > m_enable;
	std::vector< GLenum > m_disable;

	std::vector< bool > m_enableTEMP;
	std::vector< bool > m_disableTEMP;

	std::vector< Uploadable* > m_uniforms;

	std::function<void(Renderable* ) >* p_perRenderableFunction;

public:
	/**
	 * @param shader ShaderProgram to be used with this RenderPass
	 * @param fbo (optional) leave empty or 0 to render to screen
	 */
	RenderPass(ShaderProgram* shader = 0, FrameBufferObject* fbo = 0);
	virtual ~RenderPass();

	virtual void clearBits(); //!< uses glClear to clear all Bitfields omitted using addClearBit, e.g. GL_CLEAR_COLOR
	virtual void enableStates(); //!< uses glEnable to enable all states omitted using addEnable (if currently not enabled), e.g. GL_DEPTH_TEST
	virtual void disableStates(); //!< uses glDisable to disable all states omitted using addDisable (if currently enabled), e.g. GL_DEPTH_TEST

	virtual void preRender(); //!< executed before looping over all added Renderables, virtual method that may be overridden in a derived class  
	virtual void uploadUniforms(); //!< @deprecated calls all Uniform objects omitted using addUniform to upload their values, executed per Renderable. Currently kinda outdated/deprecated, should be revised
	virtual void render(); //!< execute this renderpass
	virtual void postRender(); //!< executed after looping over all Renderables, virtual method that may be overridden in a derived class
	virtual void restoreStates(); //!< resores all OpenGL states that were altered by enableStates and disableStates

	void setPerRenderableFunction(std::function<void(Renderable*)>* perRenderableFunction); //!< set pointer to a std::function object that will be called with each Renderable before calling its draw() method

	void setViewport(int x, int y, int width, int height); //!< if set, glViewport will be called with these values before rendering (if RenderPass is constructed with a FBO, it is initialized to the FBO's dimensions)
	void setClearColor(float r, float g, float b, float a = 1.0f); //!< if GL_COLOR_BUFFER_BIT is omitted to addClearBit, this color is omitted to glClearColor before glClear is called

	void setFrameBufferObject(FrameBufferObject* fbo);
	void setShaderProgram(ShaderProgram* shaderProgram);

	void addRenderable(Renderable* renderable); //!< add a pointer to a Renderable that should be drawn within this RenderPass
	void removeRenderable( Renderable* renderable );
	void clearRenderables(); //!< clear the list of pointers of Renderables

	std::vector< Renderable* > getRenderables(); //!< retrieve set of all Renderables that should be drawn within this RenderPass

	FrameBufferObject* getFrameBufferObject();
	ShaderProgram* getShaderProgram();

	void addClearBit(GLbitfield clearBit); //!< add a bitfield that should be cleared before drawing the Renderables, e.g. GL_COLOR_BUFFER_BIT
	void addEnable(GLenum state); //!< add an OpenGL state that should be enabled  before drawing the Renderables, e.g. GL_BLEND or GL_DEPTH_TEST
	void addDisable(GLenum state);//!< add an OpenGL state that should be disabled before drawing the Renderables, e.g. GL_BLEND or GL_DEPTH_TEST

	void addUniform(Uploadable* uniform); //!< @deprecated add an Uploadable that should be called before drawing a Renderable 

	void removeEnable(GLenum state);
	void removeDisable(GLenum state);
	void removeClearBit(GLbitfield clearBit);
};

#endif
