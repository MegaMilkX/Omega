#pragma once

#include "gpu/pass/gpu_pass.hpp"
#include "gpu/gpu_util.hpp"


class gpuClearPass : public gpuPass {
    gfxm::vec4 color;
public:
    gpuClearPass(const gfxm::vec4& color)
    : gpuPass(PASS_FLAG_CLEAR_PASS)
    , color(color)
    {}

    void onDraw(gpuPassInstance* inst, gpuRenderTargetMap* target_map, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) override {
        bindFramebuffer(inst, target_map);
        bindDrawBuffers(inst, target_map);

        glClearColor(color.x, color.y, color.z, color.w);
        glClearDepthf(FLT_MAX);
        // TODO: STENCIL BUFFER, OPTIONAL DEPTH, COLOR
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gpuFrameBufferUnbind();
    }
};

