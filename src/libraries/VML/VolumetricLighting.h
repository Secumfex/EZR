//
// Created by Windrian on 26.01.2016.
//

#ifndef EZR_VOLUMETRICLIGHTING_H
#define EZR_VOLUMETRICLIGHTING_H

#include <algorithm>
#include <Rendering/RenderPass.h>

class VolumetricLighting {

public:
    VolumetricLighting(int width, int height);
    ~VolumetricLighting();

    void setupNoiseTexture();
    void update(glm::mat4 &cameraView, glm::vec3 &cameraPos, glm::mat4 &lightView, glm::mat4 &lightProjection);

    // shader variables
    ShaderProgram* _raymarchingShader;
    RenderPass* _raymarchingRenderPass;

    // sampler
    FrameBufferObject* _raymarchingFBO;

    // other variables
    float _width;
	float _height;
    float _blockSize;
    float _blockSide;
    int _radiocity;
    float _scatterProbability;

    // rendering
    Quad _screenfillingQuad;

};


#endif //EZR_VOLUMETRICLIGHTING_H
