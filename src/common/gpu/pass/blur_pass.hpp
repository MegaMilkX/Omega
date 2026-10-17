#pragma once

#include "gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/render_bucket.hpp"
#include "math/gfxm.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "resource/resource.hpp"


class gpuBlurPass : public gpuPass {
    std::string source_name;
public:
    gpuBlurPass(const char* source, const char* target) {
        setColorTarget("Color", target);
        addColorSource("Color", source);

        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/blur"));
    }

    void onDraw(gpuPassInstance* inst, gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, int fb_idx, const DRAW_PARAMS& params) override {
        gpuDbgScopedGroup dbg_grp(getFullName());
        bindFramebuffer(inst, target, fb_idx, params);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        bindDefaultSamplerSet(target, inst);

        bindDefaultProgram();
        gpuDrawFullscreenTriangle();
        glBindVertexArray(0);

        gpuFrameBufferUnbind();
    }
};