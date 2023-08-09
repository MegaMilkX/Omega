#pragma once

#include "animation/animation.hpp"
#include "skeleton/skeleton_editable.hpp"

class animSampler {
    Animation* animation = 0;
    std::vector<int32_t> mapping;
public:
    animSampler() {}
    animSampler(Skeleton* skeleton, Animation* anim);

    void sample(AnimSample* out_samples, int sample_count, float cursor);
    void sample_normalized(AnimSample* out_samples, int sample_count, float cursor_normal);
    void sampleWithRootMotion(AnimSample* out_samples, int sample_count, float from, float to, AnimSample* rm_sample);
    void sampleWithRootMotion_normalized(AnimSample* out_samples, int sample_count, float from, float to, AnimSample* rm_sample);

    Animation* getAnimation();
};