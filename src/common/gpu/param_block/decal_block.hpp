#pragma once

#include "gpu/param_block/param_block.hpp"


class gpuDecalBlock : public gpuParamBlock {
    gfxm::vec4 color = gfxm::vec4(1, 1, 1, 1);
    gfxm::vec3 extents = gfxm::vec3(1, 1, 1);
public:
    void setColor(const gfxm::vec4& c) {
        color = c;
        markDirty();
    }
    void setExtents(const gfxm::vec3& e) {
        extents = e;
        markDirty();
    }
    const gfxm::vec4& getColor() const { return color; }
    const gfxm::vec3& getExtents() const { return extents; }
};

