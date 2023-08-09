#pragma once

#include <vector>
#include <algorithm>
#include "animation/animator/animator_sampler.hpp"

#include "animation/hitbox_sequence/hitbox_sequence.hpp"
#include "animation/audio_sequence/audio_sequence.hpp"

class animAnimatorSyncGroup {
    std::vector<animAnimatorSampler*> samplers;
    std::vector<animAnimatorSampler*> sorted_samplers;

    animAnimatorSampler* top_level_sampler = 0;

    bool has_hitbox_seq = false;
    bool has_audio_seq = false;
public:
    void addSampler(animAnimatorSampler* smp) {
        samplers.push_back(smp);
    }

    bool compile(Skeleton* skl) {
        has_hitbox_seq = false;
        has_audio_seq = false;
        sorted_samplers.resize(samplers.size());
        for (int i = 0; i < samplers.size(); ++i) {
            has_hitbox_seq = has_hitbox_seq || samplers[i]->getSequence()->getHitboxSequence().isValid();
            has_audio_seq = has_audio_seq || samplers[i]->getSequence()->getAudioSequence().isValid();

            sorted_samplers[i] = samplers[i];
        }
        return true;
    }

    bool hasHitboxSequences() const { return has_hitbox_seq; }
    bool hasAudioSequences() const { return has_audio_seq; }
    animAnimatorSampler* getTopLevelSampler() { return top_level_sampler; }

    void clearInfluence() {
        for (int i = 0; i < samplers.size(); ++i) {
            samplers[i]->total_influence = .0f;
        }
    }
    void updateLengths() {
        // Sort clips by lowest influence first
        std::sort(sorted_samplers.begin(), sorted_samplers.end(), [](const animAnimatorSampler* a, const animAnimatorSampler* b)->bool {
            return a->total_influence < b->total_influence;
        });
        // Set clip advance speeds
        if (!sorted_samplers.empty()) {
            sorted_samplers[0]->length_scaled = sorted_samplers[0]->getSequence()->length;
            for (int i = 1; i < sorted_samplers.size(); ++i) {
                auto clip_a = sorted_samplers[i - 1];
                auto clip_b = sorted_samplers[i];
                auto infl_a = clip_a->total_influence;
                auto infl_b = clip_b->total_influence;
                if (infl_b == .0f) {
                    continue;
                }
                auto n_infl_a = infl_a / (infl_a + infl_b);
                auto n_infl_b = infl_b / (infl_a + infl_b);
                auto spd_weight = n_infl_b;
                clip_a->length_scaled = gfxm::lerp(clip_a->getSequence()->length, clip_b->getSequence()->length, 1.0 - spd_weight);
                clip_b->length_scaled = gfxm::lerp(clip_a->getSequence()->length, clip_b->getSequence()->length, spd_weight);
            }
        }
        top_level_sampler = 0;
        if (!sorted_samplers.empty()) {
            top_level_sampler = sorted_samplers[sorted_samplers.size() - 1];
        }
    }

    void sampleClips(float dt) {
        if (getTopLevelSampler()->total_influence == .0f) {
            // No influence in this group at all, no need to advance cursors
            return;
        }
        for (int i = 0; i < sorted_samplers.size(); ++i) {
            animAnimatorSampler* sampler = sorted_samplers[i];
            sampler->sampleAndAdvance(dt, sampler->total_influence > FLT_EPSILON);
        }
    }

    size_t samplerCount() const { return samplers.size(); }
    animAnimatorSampler& operator[](int idx) { return *samplers[idx]; }
    animAnimatorSampler& getSorted(int idx) { return *(sorted_samplers[sorted_samplers.size() - 1 - idx]); }
};