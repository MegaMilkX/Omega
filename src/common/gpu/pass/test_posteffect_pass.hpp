#pragma once

#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_util.hpp"


class gpuTestPosteffectPass : public gpuPass {
public:
    gpuTestPosteffectPass(const char* source, const char* target, const char* shader_path) {
        setColorTarget("Color", target);
        addColorSource("Color", source);

        addBaseShaderSet(loadResource<gpuShaderSet>(shader_path));
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
