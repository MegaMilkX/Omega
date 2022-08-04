#pragma once

#include "animation/animator/anim_unit.hpp"

#include "handle/handle.hpp"
#include "animation/animation.hpp"
#include "animation/animation_sampler.hpp"

#include "animation/animator/animator_sampler.hpp"

class animUnitSingle : public animUnit {
    animAnimatorSampler* sampler_;
public:
    animUnitSingle() {}

    void setSampler(animAnimatorSampler* smp) { sampler_ = smp; }

    bool isAnimFinished() const override { return sampler_->cursor >= sampler_->seq->getSkeletalAnimation()->length; }

    void updateInfluence(AnimatorEd* animator, float infl) override {
        sampler_->propagateInfluence(infl);
    }
    void update(AnimatorEd* animator, animSampleBuffer* samples, float dt) override {
        // TODO: This copy should be unnecessary
        samples->copy(sampler_->samples);
    }
    bool compile(sklSkeletonEditable* skl) override {
        if (!sampler_) {
            assert(false);
            return false;
        }
        return true;
    }
};