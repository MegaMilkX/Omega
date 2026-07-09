#pragma once

#include "gpu_pass.hpp"
#include "gpu/gpu_util.hpp"
#include "gpu/ibl_maps.hpp"
#include "gpu/common_resources.hpp"


class EnvironmentIBLPass : public gpuPass {
public:
    EnvironmentIBLPass();
    void onDraw(gpuPassInstance* inst, gpuRenderTargetMap* target_map, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) override;
};