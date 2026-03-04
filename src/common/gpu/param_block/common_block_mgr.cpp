#include "common_block_mgr.hpp"

#include "gpu/render/uniform.hpp"


void gpuCommonBlockManager::onInit() {
    loc_projection = ubuf_desc->getUniform(UNIFORM_PROJECTION);
    loc_view = ubuf_desc->getUniform(UNIFORM_VIEW_TRANSFORM);
    loc_view_prev = ubuf_desc->getUniform(UNIFORM_VIEW_TRANSFORM_PREV);
    loc_camera_pos = ubuf_desc->getUniform("cameraPosition");
    loc_screenSize = ubuf_desc->getUniform("viewportSize");
    loc_zNear = ubuf_desc->getUniform("zNear");
    loc_zFar = ubuf_desc->getUniform("zFar");
    loc_vp_rect_ratio = ubuf_desc->getUniform("vp_rect_ratio");
    loc_time = ubuf_desc->getUniform("time");
    loc_gamma = ubuf_desc->getUniform("gamma");
    loc_exposure = ubuf_desc->getUniform("exposure");
}

void gpuCommonBlockManager::upload(ITEM* data, size_t count) {
    for (int i = 0; i < count; ++i) {
        auto& item = data[i];
        auto block = getBlock(item);
        item.gpu_buf->setMat4Staging(loc_projection, block->matProjection);
        item.gpu_buf->setMat4Staging(loc_view, block->matView);
        item.gpu_buf->setMat4Staging(loc_view_prev, block->matView_prev);
        item.gpu_buf->setVec3Staging(loc_camera_pos, block->cameraPosition);
        item.gpu_buf->setVec2Staging(loc_screenSize, block->viewportSize);
        item.gpu_buf->setFloatStaging(loc_zNear, block->zNear);
        item.gpu_buf->setFloatStaging(loc_zFar, block->zFar);
        item.gpu_buf->setVec4Staging(loc_vp_rect_ratio, block->vp_rect_ratio);
        item.gpu_buf->setFloatStaging(loc_time, block->time);
        item.gpu_buf->setFloatStaging(loc_gamma, block->gamma);
        item.gpu_buf->setFloatStaging(loc_exposure, block->exposure);
        item.gpu_buf->upload();
    }
}

