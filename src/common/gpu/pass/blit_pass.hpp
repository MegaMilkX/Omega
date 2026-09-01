#pragma once



#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_util.hpp"


class gpuBlitPass : public gpuPass {
public:
    gpuBlitPass(const char* source, const char* target) {
        addColorSource("Source", source);
        setColorTarget("Color", target);

        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/blit"));
    }

    void onDraw(gpuPassInstance* inst, gpuRenderTargetMap* target_map, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) override {
        bindFramebuffer(inst, target_map, params);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        glEnable(GL_BLEND);
        gpuSetBlending(blend_mode);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        //glBlendEquation(GL_FUNC_ADD);
        
        bindDefaultSamplerSet(target_map->getTarget(), inst);

        bindDefaultProgram();
        gpuDrawFullscreenTriangle();
        glBindVertexArray(0);

        gpuFrameBufferUnbind();
    }
};

class gpuDepthMergePass : public gpuPass {
public:
    gpuDepthMergePass(const char* source, const char* target) {
        addColorSource("Source", source);
        setDepthTarget(target);

        addBaseShaderSet(loadResource<gpuShaderSet>("core/shaders/merge_depth"));
    }

    void onDraw(gpuPassInstance* inst, gpuRenderTargetMap* target_map, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) override {
        bindFramebuffer(inst, target_map, params);

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);

        glEnable(GL_BLEND);
        gpuSetBlending(blend_mode);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        //glBlendEquation(GL_FUNC_ADD);

        bindDefaultSamplerSet(target_map->getTarget(), inst);

        bindDefaultProgram();
        gpuDrawFullscreenTriangle();
        glBindVertexArray(0);

        gpuFrameBufferUnbind();
    }
};
