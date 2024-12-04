#pragma once

#include "gpu_geometry_pass.hpp"


class gpuWireframePass : public gpuGeometryPass {
public:
    void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id, const DRAW_PARAMS& params) override;
};