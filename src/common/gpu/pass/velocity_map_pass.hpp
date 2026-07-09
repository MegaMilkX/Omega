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

    void onDraw(gpuPassInstance* inst, gpuRenderTargetMap* target_map, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) override {
        bindFramebuffer(inst, target_map);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_ONE, GL_ONE);

        bindDefaultSamplerSet(target_map->getTarget(), inst);

        bindDefaultProgram();
        gpuDrawFullscreenTriangle();
        glBindVertexArray(0);

        gpuFrameBufferUnbind();
    }
};

