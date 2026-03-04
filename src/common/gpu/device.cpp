#include "device.hpp"



gpuSharedResources* gpuDevice::getSharedResources() {
    if (!shared) {
        shared.reset(new gpuSharedResources);
    }
    return shared.get();
}

