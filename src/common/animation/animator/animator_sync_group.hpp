#pragma once

#include <vector>
#include <algorithm>
#include "animation/animator/animator_sampler.hpp"


class animAnimatorSyncGroup {
    std::vector<std::unique_ptr<animAnimatorSampler>> samplers;
    std::vector<animAnimatorSampler*> sorted_samplers;
public:
    animAnimatorSampler* addSampler() {
        auto ptr = new animAnimatorSampler;
        samplers.push_back(std::unique_ptr<animAnimatorSampler>(ptr));
        return ptr;
    }

    bool compile(sklSkeletonEditable* skl) {
        sorted_samplers.resize(samplers.size());
        for (int i = 0; i < samplers.size(); ++i) {
            if (!samplers[i]->seq.isValid()) {
                assert(false);
                return false;
            }
            samplers[i]->sampler = animSampler(skl, samplers[i]->seq->getSkeletalAnimation().get());
            samplers[i]->samples.init(skl);

            sorted_samplers[i] = samplers[i].get();
        }
        return true;
    }

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
            sorted_samplers[0]->length_scaled = sorted_samplers[0]->seq->getSkeletalAnimation()->length;
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
                clip_a->length_scaled = gfxm::lerp(clip_a->seq->getSkeletalAnimation()->length, clip_b->seq->getSkeletalAnimation()->length, 1.0 - spd_weight);
                clip_b->length_scaled = gfxm::lerp(clip_a->seq->getSkeletalAnimation()->length, clip_b->seq->getSkeletalAnimation()->length, spd_weight);
            }
        }
    }

    void sampleClips(float dt) {
        for (int i = 0; i < sorted_samplers.size(); ++i) {
            animAnimatorSampler* sampler = sorted_samplers[i];
            sampler->sampleAndAdvance(dt, sampler->total_influence > FLT_EPSILON);
        }
    }

    size_t samplerCount() const { return samplers.size(); }
    animAnimatorSampler& operator[](int idx) { return *samplers[idx].get(); }
    animAnimatorSampler& getSorted(int idx) { return *(sorted_samplers[sorted_samplers.size() - 1 - idx]); }
};