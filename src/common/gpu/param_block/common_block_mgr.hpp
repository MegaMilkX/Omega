#pragma once

#include "gpu/param_block/common_block.hpp"


class gpuCommonBlockManager : public gpuParamBlockMgr_T<gpuCommonBlock> {
    int loc_projection = -1;
    int loc_view = -1;
    int loc_view_prev = -1;
    int loc_camera_pos = -1;
    int loc_camera_pos_prev = -1;
    int loc_screenSize = -1;
    int loc_zNear = -1;
    int loc_zFar = -1;
    int loc_vp_rect_ratio = -1;
    int loc_time = -1;
    int loc_gamma = -1;
    int loc_exposure = -1;
public:
    void onInit() override;
    void upload(ITEM* data, size_t count) override;
};