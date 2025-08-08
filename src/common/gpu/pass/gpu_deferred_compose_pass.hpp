#pragma once

#include "gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/render_bucket.hpp"
#include "math/gfxm.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "resource/resource.hpp"


class gpuDeferredComposePass : public gpuPass {
    gpuShaderProgram* prog = 0;
public:
    gpuDeferredComposePass() {
        setColorTarget("Final", "Final");
        addColorSource("Albedo", "Albedo");
        addColorSource("Lightness", "Lightness");
        addColorSource("Emission", "Emission");
        addColorSource("AmbientOcclusion", "AmbientOcclusion");

        prog = addShader(resGet<gpuShaderProgram>("shaders/postprocess/pbr_compose.glsl"));
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