#pragma once

#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_util.hpp"


class gpuFogPass : public gpuPass {
    gpuShaderProgram* prog = 0;
    IBLMaps ibl_maps;
public:
    gpuFogPass(const char* target) {
        setColorTarget("Color", target);
        addColorSource("Depth", "Depth");

        prog = addShader(resGet<gpuShaderProgram>("core/shaders/fog.glsl"));

        ibl_maps = loadIBLMapsFromHDRI(
            "cubemaps/hdri/belfast_sunset_puresky_1k.hdr"
            //"cubemaps/hdri/studio_small_02_1k.hdr"
            //"cubemaps/hdri/2/moonless_golf_2k.hdr"
            //"cubemaps/hdri/3/mud_road_puresky_2k.hdr"
            //"cubemaps/hdri/3/overcast_soil_puresky_2k.hdr"
            //"cubemaps/hdri/rogland_clear_night_2k.hdr"
            //""
        );
        addTexture("texCubemapIrradiance", ibl_maps.irradiance, SHADER_SAMPLER_CUBE_MAP);
        addTexture("texCubemapEnvironment", ibl_maps.environment, SHADER_SAMPLER_CUBE_MAP);
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

