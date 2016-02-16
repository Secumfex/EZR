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

    ShaderProgram* _raymarchingShader;
    RenderPass* _raymarchingRenderPass;

    FrameBufferObject* _raymarchingFBO;

    Quad _screenfillingQuad;

    float _width = 800;
    float _height = 600;
    float _blockSize = 64;
    float _blockSide = 8;
    int _radiocity = 10000000.0f;
    float _scatterProbability = 0.02f;
};


#endif //EZR_VOLUMETRICLIGHTING_H
