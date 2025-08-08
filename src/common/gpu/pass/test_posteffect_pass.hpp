#pragma once

#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_util.hpp"


class gpuTestPosteffectPass : public gpuPass {
    gpuShaderProgram* prog = 0;
public:
    gpuTestPosteffectPass(const char* source, const char* target, const char* shader_path) {
        setColorTarget("Color", target);
        addColorSource("Color", source);

        prog = addShader(resGet<gpuShaderProgram>(shader_path));
    }

    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) override {
        gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, 0, 0, 0, 0, 0, 0, 0, 0, };
        glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);
        
        gpuBindSamplers(target, this, getSamplerSet(0));

        glUseProgram(prog->getId());
        gpuDrawFullscreenTriangle();
        glBindVertexArray(0);

        gpuFrameBufferUnbind();
    }
};
