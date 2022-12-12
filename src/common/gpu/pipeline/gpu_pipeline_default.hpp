#pragma once


#include "gpu/gpu_pipeline.hpp"
#include "gpu/render/uniform.hpp"
#include "platform/platform.hpp"


class gpuPipelineDefault : public gpuPipeline {
public:
    gpuPipelineDefault() {
        addColorRenderTarget("Albedo", GL_RGB);
        addDepthRenderTarget("Depth");

        auto tech = createTechnique("Normal", 1);
        tech->getPass(0)->setColorTarget("Albedo", "Albedo");
        tech->getPass(0)->setDepthTarget("Depth");
        tech = createTechnique("Decals", 1);
        tech->getPass(0)->setTargetSampler("Depth");
        tech->getPass(0)->setColorTarget("Albedo", "Albedo");
        tech = createTechnique("VFX", 1);
        tech->getPass(0)->setColorTarget("Albedo", "Albedo");
        tech->getPass(0)->setDepthTarget("Depth");
        tech = createTechnique("GUI", 1);
        tech->getPass(0)->setColorTarget("Albedo", "Albedo");
        tech->getPass(0)->setDepthTarget("Depth");
        tech = createTechnique("Debug", 1);
        tech->getPass(0)->setColorTarget("Albedo", "Albedo");
        tech->getPass(0)->setDepthTarget("Depth");

        compile();

        createUniformBufferDesc(UNIFORM_BUFFER_CAMERA_3D)
            ->define(UNIFORM_PROJECTION, UNIFORM_MAT4)
            .define(UNIFORM_VIEW_TRANSFORM, UNIFORM_MAT4)
            .compile();
        createUniformBufferDesc(UNIFORM_BUFFER_TIME)
            ->define(UNIFORM_TIME, UNIFORM_FLOAT)
            .compile();
        createUniformBufferDesc(UNIFORM_BUFFER_MODEL)
            ->define(UNIFORM_MODEL_TRANSFORM, UNIFORM_MAT4)
            .compile();
        createUniformBufferDesc(UNIFORM_BUFFER_DECAL)
            ->define("boxSize", UNIFORM_VEC3)
            .define("RGBA", UNIFORM_VEC4)
            .define("screenSize", UNIFORM_VEC2)
            .compile();
    }
};