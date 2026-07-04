#pragma once

#include "gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/render_bucket.hpp"
#include "math/gfxm.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "resource/resource.hpp"


class gpuDeferredComposePass : public gpuPass {
public:
    gpuDeferredComposePass() {
        setColorTarget("Final", "Final");
        addColorSource("Albedo", "Albedo");
        addColorSource("Lightness", "Lightness");
        addColorSource("AmbientOcclusion", "AmbientOcclusion");

        addBaseShaderSet(loadResource<gpuShaderSet>("shaders/postprocess/pbr_compose"));
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