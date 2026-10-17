#pragma once

#include "gpu_geometry_pass.hpp"


class gpuWireframePass : public gpuGeometryPass {
public:
    gpuWireframePass();
    void onDraw(gpuPassInstance* inst, gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, int fb_idx, const DRAW_PARAMS& params) override;
};