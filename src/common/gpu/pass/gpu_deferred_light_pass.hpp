#pragma once

#include "gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/render_bucket.hpp"
#include "gpu/gpu_cube_map.hpp"
#include "math/gfxm.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "resource/resource.hpp"


class gpuDeferredLightPass : public gpuPass {
    gpuShaderProgram* prog_pbr_direct_light = 0;
    gpuShaderProgram* prog_pbr_light = 0;
    HSHARED<gpuCubeMap> cube_map_shadow;
    
    GLuint shadow_vao = 0;
    GLuint cube_capture_fbo = 0;

    void gpuDrawShadowCubeMap(gpuRenderTarget* target, gpuRenderBucket* bucket, const gfxm::vec3& eye, gpuCubeMap* cubemap);
public:
    gpuDeferredLightPass() {
        setColorTarget("Lightness", "Lightness");
        addColorSource("Albedo", "Albedo");
        addColorSource("Position", "Position");
        addColorSource("Normal", "Normal");
        addColorSource("Metalness", "Metalness");
        addColorSource("Roughness", "Roughness");
        addColorSource("Emission", "Emission");

        prog_pbr_direct_light = addShader(resGet<gpuShaderProgram>("shaders/postprocess/pbr_direct_light.glsl"));
        prog_pbr_light = addShader(resGet<gpuShaderProgram>("shaders/postprocess/pbr_light.glsl"));

        cube_map_shadow.reset_acquire();
        cube_map_shadow->reserve(1024, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT);

        addTexture("ShadowCubeMap", cube_map_shadow->getId(), SHADER_SAMPLER_CUBE_MAP);

        glGenVertexArrays(1, &shadow_vao);

        glGenFramebuffers(1, &cube_capture_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, cube_capture_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, cube_map_shadow->getId(), 0);
        GLenum draw_buffers[GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERR("Shadow cube map fbo incomplete!");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    ~gpuDeferredLightPass() {
        glDeleteFramebuffers(1, &cube_capture_fbo);

        glDeleteVertexArrays(1, &shadow_vao);
    }

    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id, const DRAW_PARAMS& params) override {
        for (auto& l : bucket->lights_direct) {
            gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glEnable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_STENCIL_TEST);
            glDisable(GL_LINE_SMOOTH);
            glDepthMask(GL_TRUE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, 0, 0, 0, 0, 0, 0, 0, 0, };
            glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);

            gpuBindSamplers(target, this, getSamplerSet(0));

            glUseProgram(prog_pbr_direct_light->getId());
            prog_pbr_direct_light->setUniform3f("camPos", gfxm::inverse(params.view)[3]);
            glViewport(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);
            glScissor(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);
            prog_pbr_direct_light->setUniform3f("lightDir", l.direction);
            prog_pbr_direct_light->setUniform3f("lightColor", l.color);
            prog_pbr_direct_light->setUniform1f("lightIntensity", l.intensity);
            gpuDrawFullscreenTriangle();
            // NOTE: If we do not unbind the program here, the driver will complain about the shadow sampler
            // texture format when it is changed, but the program bound is still this one
            glUseProgram(0);
        }

        for (auto& l : bucket->lights_omni) {
            gpuDrawShadowCubeMap(target, bucket, l.position, cube_map_shadow.get());
            glBindVertexArray(0);

            gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glEnable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_STENCIL_TEST);
            glDisable(GL_LINE_SMOOTH);
            glDepthMask(GL_TRUE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, 0, 0, 0, 0, 0, 0, 0, 0, };
            glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);

            gpuBindSamplers(target, this, getSamplerSet(1));

            glUseProgram(prog_pbr_light->getId());
            prog_pbr_light->setUniform3f("camPos", gfxm::inverse(params.view)[3]);
            glViewport(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);
            glScissor(params.viewport_x, params.viewport_y, params.viewport_width, params.viewport_height);
            prog_pbr_light->setUniform3f("lightPos", l.position);
            prog_pbr_light->setUniform3f("lightColor", l.color);
            prog_pbr_light->setUniform1f("lightIntensity", l.intensity);
            gpuDrawFullscreenTriangle();
            // NOTE: If we do not unbind the program here, the driver will complain about the shadow sampler
            // texture format when it is changed, but the program bound is still this one
            glUseProgram(0);
        }
        
        glBindVertexArray(0);
        
        gpuFrameBufferUnbind();
    }
};