#include "anim_unit_single.hpp"

#include "animator.hpp"


bool animUnitSingle::compile(animGraphCompileContext* ctx, AnimatorMaster* animator, Skeleton* skl) {
    if (sampler_name.empty()) {
        assert(false);
        return false;
    }
    sampler_id = animator->getSamplerId(sampler_name.c_str());
    if (sampler_id < 0) {
        assert(false);
        return false;
    }

    return true;
}