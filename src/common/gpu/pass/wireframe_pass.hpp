#pragma once

#include "gpu_geometry_pass.hpp"


class gpuWireframePass : public gpuGeometryPass {
public:
    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) override;
};