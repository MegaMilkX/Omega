#pragma once

#include "gpu_pass.hpp"
#include "gpu/gpu_util.hpp"


class gpuSSGIPass : public gpuPass {
public:
    gpuSSGIPass();
    void onDraw(gpuPassInstance* inst, gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, int fb_idx, const DRAW_PARAMS& params) override;
};


class gpuSSGIDenoisePass : public gpuPass {
public:
    gpuSSGIDenoisePass();
    void onDraw(gpuPassInstance* inst, gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, int fb_idx, const DRAW_PARAMS& params) override;
};


class gpuSSGIComposePass : public gpuPass {
public:
    gpuSSGIComposePass();
    void onDraw(gpuPassInstance* inst, gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, int fb_idx, const DRAW_PARAMS& params) override;
};