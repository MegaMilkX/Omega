#pragma once

#include "gpu/param_block/direct_light_block.hpp"


class gpuDirectLightBlockMgr : public gpuParamBlockMgr_T<gpuDirectLightBlock> {
    int loc_viewProj = -1;
    int loc_params = -1;
    int loc_lightDir = -1;
public:
    void onInit() override;
    void upload(ITEM* data, size_t count) override;
};