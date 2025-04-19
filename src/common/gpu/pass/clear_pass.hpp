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

    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id, const DRAW_PARAMS& params) override {
        bindFramebuffer(target);
        bindDrawBuffers(target);

        glClearColor(color.x, color.y, color.z, color.w);
        // TODO: STENCIL BUFFER, OPTIONAL DEPTH, COLOR
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gpuFrameBufferUnbind();
    }
};

