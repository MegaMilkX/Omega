#pragma once

#include "gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/render_bucket.hpp"


class gpuGeometryPass : public gpuPass {
public:
    gpuGeometryPass();
    void onDraw(gpuPassInstance* inst, gpuRenderTargetMap* target_map, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) override;
};