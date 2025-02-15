#pragma once



#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_util.hpp"


class gpuBlitPass : public gpuPass {
    gpuShaderProgram* prog = 0;
public:
    gpuBlitPass(const char* source, const char* target) {
        addColorSource("Source", source);
        setColorTarget("Color", target);

        prog = addShader(resGet<gpuShaderProgram>("core/shaders/blit.glsl"));
    }

    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id, const DRAW_PARAMS& params) override {
        gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        //glBlendEquation(GL_FUNC_ADD);

        GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, 0, 0, 0, 0, 0, 0, 0, 0, };
        glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);
        
        gpuBindSamplers(target, this, getSamplerSet(0));

        glUseProgram(prog->getId());
        gpuDrawFullscreenTriangle();
        glBindVertexArray(0);

        gpuFrameBufferUnbind();
    }
};
