#pragma once

#include <memory>
#include "gpu/shared_resources.hpp"


class gpuDevice {
    std::unique_ptr<gpuSharedResources> shared;
public:
    gpuSharedResources* getSharedResources();
};

