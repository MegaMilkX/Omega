#pragma once

#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/render_bucket.hpp"
#include "gpu/gpu_cube_map.hpp"
#include "math/gfxm.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "resource/resource.hpp"


class gpuSkyboxPass : public gpuPass {
    RHSHARED<gpuShaderProgram> prog_skybox;
    RHSHARED<gpuCubeMap> sky_cube_map;

public:
    gpuSkyboxPass() {
        setColorTarget("Albedo", "Final");
        setDepthTarget("Depth");

        prog_skybox = resGet<gpuShaderProgram>("shaders/postprocess/skybox.glsl");

        sky_cube_map.reset_acquire();
        ktImage posx;
        ktImage negx;
        ktImage posy;
        ktImage negy;
        ktImage posz;
        ktImage negz;
        if (!loadImage(&posx, "cubemaps/Yokohama3/posx.jpg", false)) {
            LOG_ERR("Failed to load cube map part");
        }
        if (!loadImage(&negx, "cubemaps/Yokohama3/negx.jpg", false)) {
            LOG_ERR("Failed to load cube map part");
        }
        if (!loadImage(&posy, "cubemaps/Yokohama3/posy.jpg", false)) {
            LOG_ERR("Failed to load cube map part");
        }
        if (!loadImage(&negy, "cubemaps/Yokohama3/negy.jpg", false)) {
            LOG_ERR("Failed to load cube map part");
        }
        if (!loadImage(&posz, "cubemaps/Yokohama3/posz.jpg", false)) {
            LOG_ERR("Failed to load cube map part");
        }
        if (!loadImage(&negz, "cubemaps/Yokohama3/negz.jpg", false)) {
            LOG_ERR("Failed to load cube map part");
        }
        sky_cube_map->build(
            &posx, &negx, &posy, &negy, &posz, &negz
        );
    }

    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id, const gfxm::mat4& view, const gfxm::mat4& projection) override {
        gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());
        
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, 0, 0, 0, 0, 0, 0, 0, 0, };
        glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, sky_cube_map->getId());

        glUseProgram(prog_skybox->getId());
        prog_skybox->setUniformMatrix4("matProjection", projection);
        prog_skybox->setUniformMatrix4("matView", view);

        gpuDrawCubeMapCube();

        glBindVertexArray(0);
        gpuFrameBufferUnbind();        
    }
};