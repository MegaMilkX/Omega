#pragma once

#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/common_resources.hpp"


class gpuVelocityMapPass : public gpuPass {
public:
    gpuVelocityMapPass(const char* target) {
        setColorTarget("Color", target);
        addColorSource("Position", "Position");

        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/velocity_map"));
    }

    void onDraw(gpuPassInstance* inst, gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, int fb_idx, const DRAW_PARAMS& params) override {
        gpuDbgScopedGroup dbg_grp(getFullName());
        bindFramebuffer(inst, target, fb_idx, params);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_ONE, GL_ONE);

        bindDefaultSamplerSet(target, inst);

        bindDefaultProgram();
        gpuDrawFullscreenTriangle();
        glBindVertexArray(0);

        gpuFrameBufferUnbind();
    }
};

