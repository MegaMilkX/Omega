#pragma once

#include "animator_component.hpp"
#include "animation/hitbox_sequence/hitbox_seq_sample_buffer.hpp"


class animAnimatorHitboxComponent : public animAnimatorComponent {
public:
    hitboxSeqSampleBuffer sample_buf;

    void onUpdate(animAnimatorSyncGroup* group, float cursor_from, float cursor_to) override {
        auto& sampler = group->getSorted(0);
        auto& seq = sampler.seq;
        auto tl = seq->getGenericTimeline<hitboxSequence>();
        if (tl) {
            tl->sample(skl_inst, sample_buf.data(), sample_buf.count(), cursor_from, cursor_to);
        }
    }
};