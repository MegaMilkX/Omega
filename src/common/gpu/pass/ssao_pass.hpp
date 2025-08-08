#pragma once

#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include <random>


class gpuSSAOPass : public gpuPass {
    static const int KERNEL_SAMPLE_COUNT = 64;

    gpuShaderProgram* prog = 0;
    gfxm::vec3 sample_kernel[KERNEL_SAMPLE_COUNT];
    HSHARED<gpuTexture2d> noise_texture;
public:
    gpuSSAOPass(const char* src_worldpos, const char* src_normal, const char* target) {
        addColorSource("WorldPos", src_worldpos);
        addColorSource("Normal", src_normal);
        setColorTarget("AO", target);

        prog = addShader(resGet<gpuShaderProgram>("core/shaders/ssao.glsl"));

        std::uniform_real_distribution<float> randomf(.0, 1.);
        std::default_random_engine generator;
        for (int i = 0; i < KERNEL_SAMPLE_COUNT; ++i) {
            gfxm::vec3 s(
                randomf(generator) * 2.f - 1.f,
                randomf(generator) * 2.f - 1.f,
                randomf(generator)
            );
            s = gfxm::normalize(s);
            s *= randomf(generator);
            float scale = i / (float)KERNEL_SAMPLE_COUNT;
            scale = gfxm::lerp(.1f, 1.f, scale * scale);
            s *= scale;
            sample_kernel[i] = s;
        }

        const int NOISE_SIDE = 4;
        gfxm::vec3 ssao_noise[NOISE_SIDE * NOISE_SIDE];
        for (int i = 0; i < NOISE_SIDE * NOISE_SIDE; ++i) {
            gfxm::vec3 n(
                randomf(generator) * 2.f - 1.f,
                randomf(generator) * 2.f - 1.f,
                .0f
            );
            ssao_noise[i] = n;
        }
        noise_texture.reset_acquire();
        noise_texture->setData(&ssao_noise, NOISE_SIDE, NOISE_SIDE, 3, IMAGE_CHANNEL_FLOAT);
        noise_texture->setFilter(GPU_TEXTURE_FILTER_NEAREST);
        noise_texture->setWrapMode(GPU_TEXTURE_WRAP_REPEAT);
        addTexture("texNoise", noise_texture->getId(), SHADER_SAMPLER_TEXTURE2D);
    }

    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) override {
        gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glBlendEquation(GL_FUNC_ADD);

        GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, 0, 0, 0, 0, 0, 0, 0, 0, };
        glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);

        gpuBindSamplers(target, this, getSamplerSet(0));

        glUseProgram(prog->getId());
        
        // TODO:
        glUniform3fv(glGetUniformLocation(prog->getId(), "kernel"), 64, (float*)sample_kernel);
        
        gpuDrawFullscreenTriangle();
        glBindVertexArray(0);

        gpuFrameBufferUnbind();
    }
};

