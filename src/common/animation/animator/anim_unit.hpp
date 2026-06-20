#pragma once

#include <stdint.h>
#include "animation/animation_sample_buffer.hpp"

#include "animation/animator/animator_instance.hpp"


struct animGraphCompileContext {
    int fsm_count = 0;
    int blend_tree_count = 0;
};


class AnimMachine;
class AnimMachineInstance;
class animUnit {
protected:
    uint32_t current_loop_cycle = 0;
public:
    virtual ~animUnit() {}

    virtual bool isAnimFinished(AnimMachineInstance* anim_inst) const { return false; };

    virtual void updateInfluence(AnimMachine* master, AnimMachineInstance* anim_inst, float infl) = 0;
    virtual void update(AnimMachineInstance* anim_inst, animSampleBuffer* samples, float dt) = 0;
    virtual bool compile(animGraphCompileContext* ctx, AnimMachine* animator, Skeleton* skl) = 0;
    virtual void prepareInstance(AnimMachineInstance* inst) = 0;
};