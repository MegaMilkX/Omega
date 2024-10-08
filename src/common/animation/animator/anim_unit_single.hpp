#pragma once

#include "animation/animator/anim_unit.hpp"

#include "handle/handle.hpp"
#include "animation/animation.hpp"
#include "animation/animation_sampler.hpp"

#include "animation/animator/animator_sampler.hpp"

class animUnitSingle : public animUnit {
    std::string sampler_name;
    int sampler_id = -1;
public:
    animUnitSingle() {}

    void setSampler(const char* sampler) { sampler_name = sampler; }

    bool isAnimFinished(AnimatorInstance* anim_inst) const override { 
        return anim_inst->getSampler(sampler_id)->cursor >= anim_inst->getSampler(sampler_id)->getSequence()->length;
    }

    void updateInfluence(AnimatorMaster* master, AnimatorInstance* anim_inst, float infl) override {
        anim_inst->getSampler(sampler_id)->propagateInfluence(infl);
    }
    void update(AnimatorInstance* anim_inst, animSampleBuffer* samples, float dt) override {
        // TODO: This copy should be unnecessary
        samples->copy(anim_inst->getSampler(sampler_id)->samples);
    }
    bool compile(animGraphCompileContext* ctx, AnimatorMaster* animator, Skeleton* skl) override;

    void prepareInstance(AnimatorInstance* inst) {
        // nothing needs to be done here
    }
};