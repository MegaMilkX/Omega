#pragma once

#include "animation/animation_sample_buffer.hpp"


inline void animBlendSamples(animSampleBuffer& from, animSampleBuffer& to, animSampleBuffer& result, float factor) {
    if (from.count() != to.count() && to.count() != result.count()) {
        assert(false);
        return;
    }
    for (int i = 0; i < from.count(); ++i) {
        auto& s0 = from[i];
        auto& s1 = to[i];
        result[i].t = gfxm::lerp(s0.t, s1.t, factor);
        result[i].s = gfxm::lerp(s0.s, s1.s, factor);
        result[i].r = gfxm::slerp(s0.r, s1.r, factor);
    }

    if (!from.has_root_motion && !to.has_root_motion) {
        return;
    }
    if (from.has_root_motion && to.has_root_motion) {
        result.getRootMotionSample().t = gfxm::lerp(from.getRootMotionSample().t, to.getRootMotionSample().t, factor);
        result.getRootMotionSample().r = gfxm::slerp(from.getRootMotionSample().r, to.getRootMotionSample().r, factor);
        return;
    }
    if (from.has_root_motion) {
        result.getRootMotionSample().t = gfxm::lerp(from.getRootMotionSample().t, gfxm::vec3(0, 0, 0), factor);
        result.getRootMotionSample().r = gfxm::slerp(from.getRootMotionSample().r, gfxm::quat(0, 0, 0, 1), factor);
    }
    else {
        result.getRootMotionSample().t = gfxm::lerp(gfxm::vec3(0, 0, 0), to.getRootMotionSample().t, factor);
        result.getRootMotionSample().r = gfxm::slerp(gfxm::quat(0, 0, 0, 1), to.getRootMotionSample().r, factor);
    }
}