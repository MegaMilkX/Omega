#pragma once

#include "gpu/param_block/direct_light_block.hpp"


class gpuDirectLightBlockMgr : public gpuParamBlockMgr_T<gpuDirectLightBlock> {
    int loc_view = -1;
    int loc_proj = -1;
public:
    void onInit() override;
    void upload(ITEM* data, size_t count) override;
};