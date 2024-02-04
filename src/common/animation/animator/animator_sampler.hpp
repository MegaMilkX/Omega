#pragma once

#include "animation/animation.hpp"
#include "animation/animation_sampler.hpp"
#include "animation/animation_sample_buffer.hpp"

#include "animation/animator/animator_sequence.hpp"

#include "animation/hitbox_sequence/hitbox_seq_sample_buffer.hpp"


struct animAnimatorSampler {
private:
    RHSHARED<Animation> anm;
public:
    animSampler         sampler;
    animSampleBuffer    samples;
    float cursor_prev = .0f;
    float cursor = .0f;
    float total_influence = .0f;
    float length_scaled = .0f;

    hitboxCmdBuffer hitbox_cmd_buffer;

    void setSequence(const RHSHARED<Animation>& s) {
        this->anm = s;
        const auto& hit_seq = anm->getHitboxSequence();
        if (hit_seq) {
            hitbox_cmd_buffer.resize(hit_seq->tracks.size());
        }
    }
    Animation* getSequence() { return anm.get(); }

    bool compile(Skeleton* skl) {
        if (!anm.isValid()) {
            assert(false);
            return false;
        }
        sampler = animSampler(skl, anm.get());
        samples.init(skl);
    }

    void propagateInfluence(float influence) {
        total_influence += influence;
    }

    void sampleAndAdvance(float dt, bool sample_new) {
        auto anim = anm.get();

        if (cursor > anim->length) {
            cursor = fmodf(cursor, anim->length);
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
    void advanceOnly(float dt) {
        auto anim = anm.get();
        if (cursor > anim->length) {
            cursor = fmodf(cursor, anim->length);
        }

        cursor_prev = cursor;
        cursor += (anim->length / length_scaled) * dt * anim->fps;
    }
};