#pragma once

#include "gpu/param_block/decal_block.hpp"


class gpuDecalBlockManager : public gpuParamBlockMgr_T<gpuDecalBlock> {
    int loc_extents = -1;
    int loc_color = -1;
public:
    void onInit() override;
    void upload(ITEM* data, size_t count) override;
};

