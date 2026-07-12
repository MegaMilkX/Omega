#include "direct_light_block_mgr.hpp"


void gpuDirectLightBlockMgr::onInit() {
    loc_proj = ubuf_desc->getUniform("dirLightProj");
    loc_view = ubuf_desc->getUniform("dirLightView");
}

void gpuDirectLightBlockMgr::upload(ITEM* data, size_t count) {
    for (int i = 0; i < count; ++i) {
        auto& item = data[i];
        auto block = getBlock(item);
        item.gpu_buf->setMat4Staging(loc_proj, block->getProjection());
        item.gpu_buf->setMat4Staging(loc_view, block->getView());
        item.gpu_buf->upload();
    }
}

