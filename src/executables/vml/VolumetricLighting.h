//
// Created by Windrian on 26.01.2016.
//

#ifndef EZR_VOLUMETRICLIGHTING_H
#define EZR_VOLUMETRICLIGHTING_H

#include <Rendering/RenderPass.h>

class VolumetricLighting {

public:
    VolumetricLighting(int width, int height);
    ~VolumetricLighting();

    ShaderProgram* _raymarchingShader;
    RenderPass* _raymarchingRenderPass;

    FrameBufferObject* _raymarchingFBO;

    Quad _screenfillingQuad;

    int _numberOfSamples = 7;
    float _densityOfMedium = 0.027;
    float _scatterProbability = 0.02f;
};


#endif //EZR_VOLUMETRICLIGHTING_H
