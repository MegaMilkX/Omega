#include "transform_block_mgr.hpp"

#include "gpu/gpu.hpp" // for gpuGetPipeline()


void gpuTransformBlockManager::onInit() {
    loc_model = ubuf_desc->getUniform(UNIFORM_MODEL_TRANSFORM);
    loc_model_prev = ubuf_desc->getUniform(UNIFORM_MODEL_TRANSFORM_PREV);
}
void gpuTransformBlockManager::upload(ITEM* data, size_t count) {
    //LOG_DBG("Dirty count: " << count);
    for (int i = 0; i < count; ++i) {
        auto& item = data[i];
        auto block = getBlock(item);
        auto inter = getInternal(item);
        item.gpu_buf->setMat4Staging(loc_model, block->getTransform());
        item.gpu_buf->setMat4Staging(loc_model_prev, inter->prev_transform);
        item.gpu_buf->upload();
        inter->prev_transform = block->getTransform();
    }
}

