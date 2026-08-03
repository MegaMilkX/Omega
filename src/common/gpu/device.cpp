#include "device.hpp"

#include "platform/platform.hpp"


int gpuDevice::getUniformBlockLocation(const std::string& declname) {
    const int max_bindings = platformGeti(PLATFORM_MAX_UNIFORM_BUFFER_BINDINGS);
    if (next_uniform_block_location == max_bindings) {
        LOG_ERR("Uniform buffer binding limit reached");
        assert(false);
        return -1;
    }

    auto it = uniform_block_locations.find(declname);
    if (it == uniform_block_locations.end()) {
        it = uniform_block_locations.insert(
            std::make_pair(
                declname,
                next_uniform_block_location++
            )
        ).first;
    }
    return it->second;
}

gpuParamBlockContext* gpuDevice::getParamBlockContext() {
    return &param_block_ctx;
}
gpuSharedResources* gpuDevice::getSharedResources() {
    if (!shared) {
        shared.reset(new gpuSharedResources);
    }
    return shared.get();
}

