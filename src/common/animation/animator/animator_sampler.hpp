#pragma once

#include "animation/animation.hpp"
#include "animation/animation_sampler.hpp"
#include "animation/animation_sample_buffer.hpp"

#include "animation/animator/animator_sequence.hpp"

struct animAnimatorSampler {
    RHSHARED<animAnimatorSequence> seq;
    animSampler         sampler;
    animSampleBuffer    samples;
    float cursor_prev = .0f;
    float cursor = .0f;
    float total_influence = .0f;
    float length_scaled = .0f;

    void setSequence(const RHSHARED<animAnimatorSequence>& s) { this->seq = s; }

    void propagateInfluence(float influence) {
        total_influence += influence;
    }

    void sampleAndAdvance(float dt, bool sample_new) {
        auto anim = seq->getSkeletalAnimation();

        if (cursor > anim->length) {
            cursor -= anim->length;
        }
        if (sample_new) {
            if (anim->hasRootMotion()) {
                samples.has_root_motion = true;
                sampler.sampleWithRootMotion(samples.data(), samples.count(), cursor_prev, cursor, &samples.getRootMotionSample());
            }
            else {
                samples.has_root_motion = false;
                sampler.sample(samples.data(), samples.count(), cursor);
            }
        }
        cursor_prev = cursor;
        cursor += (anim->length / length_scaled) * dt * anim->fps;
    }
};