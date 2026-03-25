#pragma once

#include "gpu/param_block/param_block.hpp"
#include "transform_node/transform_node.hpp"


class gpuTransformBlock : public gpuParamBlock {
    gfxm::mat4 transform = gfxm::mat4(1.f);
    bool continuous = false;
public:
    TransformTicket* ticket = nullptr;
    HTransform transform_node;

    void setTransform(const gfxm::mat4& t, bool continuous_motion = true) {
        transform = t;
        if(continuous) {
            continuous = continuous_motion;
        }
        markDirty();
    }
    void markTeleported() { continuous = false; }
    bool _hasContinuousMotion() const { return continuous; }
    void _resetContinuous() { continuous = true; }
    const gfxm::mat4& getTransform() const { return transform; }
};

