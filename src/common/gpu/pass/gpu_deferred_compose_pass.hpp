#pragma once

#include "gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/render_bucket.hpp"
#include "math/gfxm.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "resource/resource.hpp"


class gpuDeferredComposePass : public gpuPass {
    RHSHARED<gpuShaderProgram> prog_pbr_compose;
public:
    gpuDeferredComposePass() {
        setColorTarget("Final", "Final");
        setTargetSampler("Albedo");
        setTargetSampler("Lightness");
        setTargetSampler("Emission");

        prog_pbr_compose = resGet<gpuShaderProgram>("shaders/postprocess/pbr_compose.glsl");
    }
    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id, const gfxm::mat4& view, const gfxm::mat4& projection) override {
        gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, 0, 0, 0, 0, 0, 0, 0, 0, };
        glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);

        static const string_id albedo_id("Albedo");
        static const string_id lightness_id("Lightness");
        static const string_id emission_id("Emission");

        int slot = prog_pbr_compose->getDefaultSamplerSlot("Albedo");
        if (slot != -1) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, target->textures[getTargetSamplerTextureIndex(albedo_id)]->getId());
        }
        slot = prog_pbr_compose->getDefaultSamplerSlot("Lightness");
        if (slot != -1) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, target->textures[getTargetSamplerTextureIndex(lightness_id)]->getId());
        }
        slot = prog_pbr_compose->getDefaultSamplerSlot("Emission");
        if (slot != -1) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, target->textures[getTargetSamplerTextureIndex(emission_id)]->getId());
        }
        
        glUseProgram(prog_pbr_compose->getId());
        gpuDrawFullscreenTriangle();
        glBindVertexArray(0);

        gpuFrameBufferUnbind();
    }
};