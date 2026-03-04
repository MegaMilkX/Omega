#pragma once

#include "gpu/param_block/param_block.hpp"


class gpuTransformBlock : public gpuParamBlock {
    gfxm::mat4 transform = gfxm::mat4(1.f);
public:
    void setTransform(const gfxm::mat4& t) {
        transform = t;
        markDirty();
    }
    const gfxm::mat4& getTransform() const { return transform; }
};

