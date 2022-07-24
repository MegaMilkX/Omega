#pragma once

#include "animation/animation.hpp"
#include "skeleton/skeleton_editable.hpp"

class Skeleton;
class animSampler {
    Animation* animation = 0;
    std::vector<int32_t> mapping;
public:
    animSampler() {}
    animSampler(sklSkeletonEditable* skeleton, Animation* anim);

    void sample(AnimSample* out_samples, int sample_count, float cursor);
    void sample_normalized(AnimSample* out_samples, int sample_count, float cursor_normal);

    Animation* getAnimation();
};