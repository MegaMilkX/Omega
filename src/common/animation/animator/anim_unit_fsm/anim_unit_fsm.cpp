#include "anim_unit_fsm.hpp"

#include "animation/animator/animator.hpp"


bool animFsmState::compile(AnimatorMaster* animator, Skeleton* skl) {
    if (!unit) {
        return false;
    }
    if (!unit->compile(animator, skl)) {
        return false;
    }

    expr_on_exit_addr = animator->compileExpr(expr_on_exit);
    for (int i = 0; i < transitions.size(); ++i) {
        transitions[i].expr_addr = animator->compileExpr(transitions[i].expression);
        if (transitions[i].expr_addr == -1) {
            return false;
        }
    }

    if (!expr_on_exit.empty()) {
        assert(expr_on_exit_addr != -1);
    }
    return true;
}


bool animUnitFsm::compile(AnimatorMaster* animator, Skeleton* skl) {
    if (states.empty() || current_state == 0) {
        assert(false);
        LOG_ERR("Animator fsm compilation: no states supplied");
        return false;
    }
    for (auto& it : states) {
        if (!it.second->compile(animator, skl)) {
            LOG_ERR("Animator fsm compilation: failed to compile state");
            return false;
        }
    }

    for (int i = 0; i < global_transitions.size(); ++i) {
        global_transitions[i].expr_addr
            = animator->compileExpr(global_transitions[i].expression);
        if (global_transitions[i].expr_addr == -1) {
            return false;
        }
    }

    latest_state_samples.init(skl);
    return true;
}
