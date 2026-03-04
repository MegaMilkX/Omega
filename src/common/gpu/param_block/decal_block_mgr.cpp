#include "decal_block_mgr.hpp"

#include "gpu/gpu.hpp" // for gpuGetPipeline()


void gpuDecalBlockManager::onInit() {
    loc_extents = ubuf_desc->getUniform("boxSize");
    loc_color = ubuf_desc->getUniform("RGBA");
}
void gpuDecalBlockManager::upload(ITEM* data, size_t count) {
    //LOG_DBG("Dirty count: " << count);
    for (int i = 0; i < count; ++i) {
        auto& item = data[i];
        auto block = getBlock(item);
        item.gpu_buf->setVec3Staging(loc_extents, block->getExtents());
        item.gpu_buf->setVec4Staging(loc_color, block->getColor());
        item.gpu_buf->upload();
    }
}

