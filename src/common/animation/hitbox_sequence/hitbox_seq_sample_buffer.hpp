#pragma once

#include "animation/hitbox_sequence/hitbox_sequence.hpp"

struct hitboxSeqSampleBuffer {
    std::vector<hitboxSeqSample> samples;

    void init(const hitboxSequence* seq) {
        samples.resize(seq->tracks.size());
    }

    size_t count() const { return samples.size(); }
    hitboxSeqSample& operator[](int index) { return samples[index]; }
    const hitboxSeqSample& operator[](int index) const { return samples[index]; }
    hitboxSeqSample* data() { return &samples[0]; }
    const hitboxSeqSample* data() const { return &samples[0]; }
};