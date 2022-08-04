#pragma once

#include <stdint.h>
#include "animation/animation_sample_buffer.hpp"


class AnimatorEd;
class animUnit {
protected:
    uint32_t current_loop_cycle = 0;
public:
    virtual ~animUnit() {}

    virtual bool isAnimFinished() const { return false; };

    virtual void updateInfluence(AnimatorEd* animator, float infl) = 0;
    virtual void update(AnimatorEd* animator, animSampleBuffer* samples, float dt) = 0;
    virtual bool compile(sklSkeletonEditable* skl) = 0;
};