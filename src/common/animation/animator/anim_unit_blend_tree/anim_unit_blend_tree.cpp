#include "anim_unit_blend_tree.hpp"

#include "animation/animator/animator.hpp"

bool animBtNodeClip::compile(AnimatorEd* animator, std::set<animBtNode*>& exec_set, int order) {
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

bool animBtNodeFrame::compile(AnimatorEd* animator, std::set<animBtNode*>& exec_set, int order) {
    // TODO
    return false;
}

bool animBtNodeBlend2::compile(AnimatorEd* animator, std::set<animBtNode*>& exec_set, int order) {
    if (in_a == nullptr || in_b == nullptr) {
        LOG_ERR("animBtNodeBlend2::compile(): input is incomplete (a or b is null)");
        assert(false);
        return false;
    }
    updatePriority(order);
    samples.init(animator->getSkeleton());
    in_a->compile(animator, exec_set, order + 1);
    in_b->compile(animator, exec_set, order + 1);
    exec_set.insert(this);
    return true;
}