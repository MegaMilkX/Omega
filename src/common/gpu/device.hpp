#pragma once

#include <memory>
#include "gpu/param_block/param_block_context.hpp"
#include "gpu/shared_resources.hpp"


class gpuDevice {
    std::map<std::string, int> uniform_block_locations;
    int next_uniform_block_location = 0;

    gpuParamBlockContext param_block_ctx;
    std::unique_ptr<gpuSharedResources> shared;

public:
    int getUniformBlockLocation(const std::string& declname);

    gpuParamBlockContext* getParamBlockContext();
    template<typename PARAM_BLOCK_T>
    PARAM_BLOCK_T* createParamBlock() { return param_block_ctx.createParamBlock<PARAM_BLOCK_T>(); }
    void destroyParamBlock(gpuParamBlock* block) { block->getMgr()->release(block); }

    gpuSharedResources* getSharedResources();
};

extern gpuDevice* gpuGetDevice();