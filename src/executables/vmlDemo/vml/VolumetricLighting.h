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

    float _width;
    float _height;
    float _blockSize;
    float _blockSide;
    int _radiocity;
    float _scatterProbability;
};


#endif //EZR_VOLUMETRICLIGHTING_H
