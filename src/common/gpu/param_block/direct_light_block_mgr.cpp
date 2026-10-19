#include "direct_light_block_mgr.hpp"


void gpuDirectLightBlockMgr::onInit() {
    loc_viewProj = ubuf_desc->getUniform("cascadeViewProj");
    loc_params = ubuf_desc->getUniform("cascadeParams");
    loc_lightDir = ubuf_desc->getUniform("lightDir");
}

void gpuDirectLightBlockMgr::upload(ITEM* data, size_t count) {
    for (int i = 0; i < count; ++i) {
        auto& item = data[i];
        auto block = getBlock(item);
        item.gpu_buf->setMat4Staging(loc_viewProj, block->cascadeViewProj, N_CASCADES);
        item.gpu_buf->setVec4Staging(loc_params, block->cascadeParams, N_CASCADES);
        item.gpu_buf->setVec4Staging(loc_lightDir, &block->lightDir, 1);
        item.gpu_buf->upload();
    }
}

