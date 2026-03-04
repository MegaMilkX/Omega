#pragma once

#include "param_block_mgr.hpp"
#include "gpu/gpu_uniform_buffer.hpp"


struct gpuParamBlock {
    gpuParamBlockManager* mgr = nullptr;
    gpuUniformBuffer* ubuf = nullptr;
    int index = 0;
    gpuParamBlockManager* getMgr() { return mgr; }
    void markDirty() {
        mgr->markDirty(this);
    }
};

