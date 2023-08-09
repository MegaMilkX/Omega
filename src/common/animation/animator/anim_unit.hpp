#pragma once

#include <stdint.h>
#include "animation/animation_sample_buffer.hpp"

#include "animation/animator/animator_instance.hpp"


class AnimatorMaster;
class animAnimatorInstance;
class animUnit {
protected:
    uint32_t current_loop_cycle = 0;
public:
    virtual ~animUnit() {}

    virtual bool isAnimFinished(animAnimatorInstance* anim_inst) const { return false; };

    virtual void updateInfluence(animAnimatorInstance* anim_inst, float infl) = 0;
    virtual void update(animAnimatorInstance* anim_inst, animSampleBuffer* samples, float dt) = 0;
    virtual bool compile(AnimatorMaster* animator, Skeleton* skl) = 0;
};