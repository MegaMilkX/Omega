#pragma once

#include "gpu/param_block/transform_block.hpp"

struct gpuInternalTransformBlock {
    gfxm::mat4 prev_transform = gfxm::mat4(1.f);
};

class gpuTransformBlockManager : public gpuParamBlockMgr_T<gpuTransformBlock, gpuInternalTransformBlock> {
    int loc_model = -1;
    int loc_model_prev = -1;
public:
    void onInit() override;
    void upload(ITEM* data, size_t count) override;
};

