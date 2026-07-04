#pragma once

#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/common_resources.hpp"


class gpuFogPass : public gpuPass {
public:
    gpuFogPass(const char* target) {
        setColorTarget("Color", target);
        addColorSource("Depth", "Depth");

        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/fog"));

        addTexture("texCubemapIrradiance", ibl_maps.irradiance, SHADER_SAMPLER_CUBE_MAP);
        addTexture("texCubemapEnvironment", ibl_maps.environment, SHADER_SAMPLER_CUBE_MAP);
    }

    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) override {
        gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        bindDefaultSamplerSet(target);

        bindDefaultProgram();
        gpuDrawFullscreenTriangle();
        glBindVertexArray(0);

        gpuFrameBufferUnbind();
    }
};

