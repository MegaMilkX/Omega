#pragma once

#include "gpu/gpu_types.hpp"
#include "gpu/types.hpp"
#include "gpu/gpu_renderable.hpp"


struct gpuRenderCmd {
    //RenderId id;
    pipe_pass_id_t pass_id;
    uint32_t program_id; // for sorting, not real graphics api id
    uint32_t state_id;
    uint32_t sampler_set_id;
    int renderable_pass_id;
    gpuRenderable* renderable;
    const gpuCompiledRenderableDesc::PassDesc* rdr_pass;
    int instance_count;
    uint32_t program; // GLuint
    float depth;
};

void gpuSetModes(const gpuRenderCmd& cmd);
void gpuSetBlending(GPU_BLEND_MODE mode);
void gpuSetBlending(const gpuRenderCmd& cmd);
void gpuBindDrawBuffers(const gpuRenderCmd& cmd);
void gpuBindProgram(const gpuRenderCmd& cmd);

struct gpuRenderCmdLightOmni {
    gfxm::vec3 position;
    gfxm::vec3 color;
    float intensity;
    bool shadow;
};
struct gpuRenderCmdLightDirect {
    gfxm::vec3 direction;
    gfxm::vec3 color;
    float intensity;
};

