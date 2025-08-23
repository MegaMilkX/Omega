#include "gpu_skybox_pass.hpp"

#include "gpu/common_resources.hpp"


gpuSkyboxPass::gpuSkyboxPass() {
    setColorTarget("Albedo", "Final");
    setDepthTarget("Depth");

    prog_skybox = addShader(resGet<gpuShaderProgram>("shaders/postprocess/skybox.glsl"));

    addTexture("cubeMap", ibl_maps.environment, SHADER_SAMPLER_CUBE_MAP);

    /*
    ibl_maps = loadIBLMapsFromCubeSides(
        "cubemaps/Yokohama3/posx.jpg",
        "cubemaps/Yokohama3/negx.jpg",
        "cubemaps/Yokohama3/posy.jpg",
        "cubemaps/Yokohama3/negy.jpg",
        "cubemaps/Yokohama3/posz.jpg",
        "cubemaps/Yokohama3/negz.jpg"
    );*/
    /*
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
    );*/
}

void gpuSkyboxPass::onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) {
    gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());
        
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, 0, 0, 0, 0, 0, 0, 0, 0, };
    glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, draw_buffers);

    gpuBindSamplers(target, this, getSamplerSet(0));

    glUseProgram(prog_skybox->getId());

    gpuDrawCubeMapCube();

    glBindVertexArray(0);
    gpuFrameBufferUnbind();        
}
