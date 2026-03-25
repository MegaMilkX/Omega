#include "param_block_mgr.hpp"

#include "param_block.hpp"

#include "gpu/gpu.hpp" // for gpuGetPipeline()
#include "gpu/gpu_renderable.hpp"


bool gpuParamBlockManager::init(gpuUniformBufferDesc* ubuf_desc) {
    if (!ubuf_desc) {
        return false;
    }
    this->ubuf_desc = ubuf_desc;
    onInit();
    return true;
}
    
gpuParamBlock* gpuParamBlockManager::allocate() {
    ITEM item = {
        .block = allocateBlock(),
        .internal = allocateInternalBlock()
    };

    item.block->mgr = this;
    item.block->index = items.size();
        
    item.gpu_buf = gpuGetPipeline()->createUniformBuffer(ubuf_desc);
    item.block->ubuf = item.gpu_buf;

    items.push_back(item);
    markDirty(item.block);
    return item.block;
}
void gpuParamBlockManager::release(gpuParamBlock* b) {
    if (b->mgr != this) {
        assert(false);
        return;
    }
    size_t di = b->index;
        
    releaseBlock(items[di].block);
    releaseInternalBlock(items[di].internal);

    gpuGetPipeline()->destroyUniformBuffer(items[di].gpu_buf);

    items.erase(items.begin() + di);
    for (int i = di; i < items.size(); ++i) {
        items[i].block->index = i;
    }
    if (di < dirty_count) {
        --dirty_count;
    }
}
    
void gpuParamBlockManager::markDirty(gpuParamBlock* b) {
    if (b->mgr != this) {
        assert(false);
        return;
    }
    if (b->index < dirty_count) {
        return;
    }
    size_t ia = dirty_count;
    size_t ib = b->index;
    std::swap(items[ia], items[ib]);
    items[ia].block->index = ia;
    items[ib].block->index = ib;
    if(ib >= ia) {
        ++dirty_count;
    }
}

void gpuParamBlockManager::attach(gpuRenderable* renderable, gpuParamBlock* block) {
    if (block->mgr != this) {
        assert(false);
        return;
    }
    auto& item = items[block->index];
    renderable->attachUniformBuffer(item.gpu_buf);
}

int gpuParamBlockManager::upload() {
    if (items.empty()) {
        return 0;
    }
    upload(&items[0], dirty_count);
    int ret = dirty_count;
    dirty_count = 0;
    return ret;
}

