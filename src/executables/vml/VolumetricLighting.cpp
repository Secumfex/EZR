//
// Created by Windrian on 26.01.2016.
//

#include "VolumetricLighting.h"

VolumetricLighting::VolumetricLighting(int width, int height) {
    _raymarchingFBO = new FrameBufferObject(width, height);
    _raymarchingFBO->addColorAttachments(1);
    _raymarchingShader = new ShaderProgram("/screenSpace/fullscreen.vert", "/vml/raymarching_gbuffer.frag");
    _raymarchingShader->update("numberOfSamples", _numberOfSamples);
    _raymarchingShader->update("tau", _densityOfMedium);
    _raymarchingShader->update("albedo", _scatterProbability);
    _raymarchingRenderPass = new RenderPass(_raymarchingShader, _raymarchingFBO);
    _raymarchingRenderPass->addRenderable(&_screenfillingQuad);
    _raymarchingRenderPass->setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    _raymarchingRenderPass->addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    _raymarchingRenderPass->addDisable(GL_DEPTH_BUFFER_BIT);
}

VolumetricLighting::~VolumetricLighting() {

}

