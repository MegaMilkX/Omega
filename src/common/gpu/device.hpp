#pragma once

#include <memory>
#include "gpu/param_block/param_block_context.hpp"
#include "gpu/shared_resources.hpp"


class gpuDevice {
    gpuParamBlockContext param_block_ctx;
    std::unique_ptr<gpuSharedResources> shared;
public:
    gpuParamBlockContext* getParamBlockContext();
    template<typename PARAM_BLOCK_T>
    PARAM_BLOCK_T* createParamBlock() { return param_block_ctx.createParamBlock<PARAM_BLOCK_T>(); }
    void destroyParamBlock(gpuParamBlock* block) { block->getMgr()->release(block); }

    gpuSharedResources* getSharedResources();
};

