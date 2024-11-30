#pragma once

#include "animation.hpp"
#include "skeleton/skeleton_editable.hpp"

class animSampleBuffer {
    std::vector<AnimSample> samples;
    AnimSample              root_motion_sample;
public:
    bool has_root_motion = false;

    void init(Skeleton* skl) {
        samples.resize(skl->boneCount());
        for (int i = 0; i < skl->boneCount(); ++i) {
            auto bone = skl->getBone(i);
            samples[i].t = bone->getLclTranslation();
            samples[i].r = bone->getLclRotation();
            samples[i].s = bone->getLclScale();
        }
    }

    void copy(const animSampleBuffer& other) {
        if (other.count() != count()) {
            assert(false);
            return;
        }
        memcpy(samples.data(), other.data(), samples.size() * sizeof(samples[0]));
        root_motion_sample = other.root_motion_sample;
        has_root_motion = other.has_root_motion;
    }

    AnimSample& getRootMotionSample() { return root_motion_sample; }
    const AnimSample& getRootMotionSample() const { return root_motion_sample; }

    void applySamples(SkeletonInstance* skl_inst) {
        for (int i = 1; i < samples.size(); ++i) {
            auto& s = samples[i];
            skl_inst->getBoneNode(i)->setTranslation(s.t);
            skl_inst->getBoneNode(i)->setRotation(s.r);
            skl_inst->getBoneNode(i)->setScale(s.s);
        }
    }

    size_t count() const { return samples.size(); }
    AnimSample& operator[](int index) { return samples[index]; }
    const AnimSample& operator[](int index) const { return samples[index]; }
    AnimSample* data() { return &samples[0]; }
    const AnimSample* data() const { return &samples[0]; }
};