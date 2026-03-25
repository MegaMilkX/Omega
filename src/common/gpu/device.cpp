#include "device.hpp"


gpuParamBlockContext* gpuDevice::getParamBlockContext() {
    return &param_block_ctx;
}
gpuSharedResources* gpuDevice::getSharedResources() {
    if (!shared) {
        shared.reset(new gpuSharedResources);
    }
    return shared.get();
}

