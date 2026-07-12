#pragma once

#include "gpu/param_block/param_block.hpp"


class gpuDirectLightBlock : public gpuParamBlock {
    gfxm::mat4 proj;
    gfxm::mat4 view;
public:
    void setView(const gfxm::mat4& v) {
        view = v;
        markDirty();
    }
    void setProjection(const gfxm::mat4& p) {
        proj = p;
        markDirty();
    }

    const gfxm::mat4& getView() const { return view; }
    const gfxm::mat4& getProjection() const { return proj; }
};