#include "expr.hpp"

#include <assert.h>
#include "log/log.hpp"
#include "animation/animator/animator.hpp"
#include "animation/animator/anim_unit_fsm/anim_unit_fsm.hpp"

param_::param_(AnimatorEd* animator, const std::string& name) {
    index = animator->getParamId(name.c_str());
}
signal_::signal_(AnimatorEd* animator, const std::string& signal_name) {
    index = animator->getSignalId(signal_name.c_str());
}
call_feedback_event_::call_feedback_event_(AnimatorEd* animator, const std::string& fb_event_name) {
    index = animator->getFeedbackEventId(fb_event_name.c_str());
}

value_ value_eval_eq(value_ l, value_ r) {
    if (l.type != r.type) { r.convert(l.type, r); }
    switch (l.type) {
    case VALUE_FLOAT: return value_(l.float_ == r.float_);
    case VALUE_FLOAT2: return value_(l.float2.x == r.float2.x && l.float2.y == r.float2.y);
    case VALUE_FLOAT3: return value_(l.float3.x == r.float3.x && l.float3.y == r.float3.y && l.float3.z == r.float3.z);
    case VALUE_INT: return value_(l.int_ == r.int_);
    case VALUE_INT2: return value_(l.int2.x == r.int2.x && l.int2.y == r.int2.y);
    case VALUE_INT3: return value_(l.int3.x == r.int3.x && l.int3.y == r.int3.y && l.int3.z == r.int3.z);
    case VALUE_BOOL: return value_(l.bool_ == r.bool_);
    default: assert(false); return value_(false);
    }
}
value_ value_eval_noteq(value_ l, value_ r) {
    if (l.type != r.type) { r.convert(l.type, r); }
    switch (l.type) {
    case VALUE_FLOAT: return value_(l.float_ != r.float_);
    case VALUE_FLOAT2: return value_(l.float2.x != r.float2.x || l.float2.y != r.float2.y);
    case VALUE_FLOAT3: return value_(l.float3.x != r.float3.x || l.float3.y != r.float3.y || l.float3.z != r.float3.z);
    case VALUE_INT: return value_(l.int_ != r.int_);
    case VALUE_INT2: return value_(l.int2.x != r.int2.x || l.int2.y != r.int2.y);
    case VALUE_INT3: return value_(l.int3.x != r.int3.x || l.int3.y != r.int3.y || l.int3.z != r.int3.z);
    case VALUE_BOOL: return value_(l.bool_ != r.bool_);
    default: assert(false); return value_(false);
    }
}
value_ value_eval_less(value_ l, value_ r) {
    if (l.type != r.type) { r.convert(l.type, r); }
    switch (l.type) {
    case VALUE_FLOAT: return value_(l.float_ < r.float_);
    case VALUE_INT: return value_(l.int_ < r.int_);
    case VALUE_BOOL: return value_(l.bool_ < r.bool_);
    default: assert(false); return value_(false);
    }
}
value_ value_eval_more(value_ l, value_ r) {
    if (l.type != r.type) { r.convert(l.type, r); }
    switch (l.type) {
    case VALUE_FLOAT: return value_(l.float_ > r.float_);
    case VALUE_INT: return value_(l.int_ > r.int_);
    case VALUE_BOOL: return value_(l.bool_ > r.bool_);
    default: assert(false); return value_(false);
    }
}
value_ value_eval_lesseq(value_ l, value_ r) {
    if (l.type != r.type) { r.convert(l.type, r); }
    switch (l.type) {
    case VALUE_FLOAT: return value_(l.float_ <= r.float_);
    case VALUE_INT: return value_(l.int_ <= r.int_);
    case VALUE_BOOL: return value_(l.bool_ <= r.bool_);
    default: assert(false); return value_(false);
    }
}
value_ value_eval_moreeq(value_ l, value_ r) {
    if (l.type != r.type) { r.convert(l.type, r); }
    switch (l.type) {
    case VALUE_FLOAT: return value_(l.float_ >= r.float_);
    case VALUE_INT: return value_(l.int_ >= r.int_);
    case VALUE_BOOL: return value_(l.bool_ >= r.bool_);
    default: assert(false); return value_(false);
    }
}
value_ value_eval_add(value_ l, value_ r) {
    if (l.type != r.type) { r.convert(l.type, r); }
    switch (l.type) {
    case VALUE_FLOAT: return value_(l.float_ + r.float_);
    case VALUE_FLOAT2: return value_(l.float2 + r.float2);
    case VALUE_FLOAT3: return value_(l.float3 + r.float3);
    case VALUE_INT: return value_(l.int_ + r.int_);
    case VALUE_INT2: return value_(l.int2 + r.int2);
    case VALUE_INT3: return value_(l.int3 + r.int3);
    case VALUE_BOOL: return value_(l.bool_ + r.bool_);
    default: assert(false); return value_(false);
    }
}
value_ value_eval_sub(value_ l, value_ r) {
    if (l.type != r.type) { r.convert(l.type, r); }
    switch (l.type) {
    case VALUE_FLOAT: return value_(l.float_ - r.float_);
    case VALUE_FLOAT2: return value_(l.float2 - r.float2);
    case VALUE_FLOAT3: return value_(l.float3 - r.float3);
    case VALUE_INT: return value_(l.int_ - r.int_);
    case VALUE_INT2: return value_(l.int2 - r.int2);
    case VALUE_INT3: return value_(l.int3 - r.int3);
    case VALUE_BOOL: return value_(l.bool_ - r.bool_);
    default: assert(false); return value_(false);
    }
}
value_ value_eval_mul(value_ l, value_ r) {
    if (l.type != r.type) { r.convert(l.type, r); }
    switch (l.type) {
    case VALUE_FLOAT: return value_(l.float_ * r.float_);
        //case VALUE_FLOAT2: return value_(l.float2 * r.float2);
        //case VALUE_FLOAT3: return value_(l.float3 * r.float3);
    case VALUE_INT: return value_(l.int_ * r.int_);
        //case VALUE_INT2: return value_(l.int2 * r.int2);
        //case VALUE_INT3: return value_(l.int3 * r.int3);
    case VALUE_BOOL: return value_(l.bool_ * r.bool_);
    default: assert(false); return value_(false);
    }
}
value_ value_eval_div(value_ l, value_ r) {
    if (l.type != r.type) {
        r.convert(l.type, r);
    }
    switch (l.type) {
    case VALUE_FLOAT: return value_(l.float_ / r.float_);
        //case VALUE_FLOAT2: return value_(l.float2 / r.float2);
        //case VALUE_FLOAT3: return value_(l.float3 / r.float3);
    case VALUE_INT: return value_(l.int_ / r.int_);
        //case VALUE_INT2: return value_(l.int2 / r.int2);
        //case VALUE_INT3: return value_(l.int3 / r.int3);
    case VALUE_BOOL: return value_(l.bool_ / r.bool_);
    default: assert(false); return value_(false);
    }
}
value_ value_eval_mod(value_ l, value_ r) {
    if (l.type != r.type) { r.convert(l.type, r); }
    switch (l.type) {
    case VALUE_FLOAT: return value_(fmod(l.float_, r.float_));
        //case VALUE_FLOAT2: return value_(l.float2 / r.float2);
        //case VALUE_FLOAT3: return value_(l.float3 / r.float3);
    case VALUE_INT: return value_(l.int_ % r.int_);
        //case VALUE_INT2: return value_(l.int2 / r.int2);
        //case VALUE_INT3: return value_(l.int3 / r.int3);
    case VALUE_BOOL: return value_(l.bool_ % r.bool_);
    default: assert(false); return value_(false);
    }
}
value_ value_eval_abs(value_ l) {
    switch (l.type) {
    case VALUE_FLOAT: return value_(fabs(l.float_));
    case VALUE_INT: return value_(abs(l.int_));
    default: assert(false); return value_(false);
    }
}
value_ value_eval(animAnimatorInstance* anim_inst, const expr_& e, value_& l, value_& r) {
    switch (e.type) {
    case EXPR_NOOP: return value_(false);
    case EXPR_EQ: return value_eval_eq(l, r);
    case EXPR_NOTEQ: return value_eval_noteq(l, r);
    case EXPR_LESS: return value_eval_less(l, r);
    case EXPR_MORE: return value_eval_more(l, r);
    case EXPR_LESSEQ: return value_eval_lesseq(l, r);
    case EXPR_MOREEQ: return value_eval_moreeq(l, r);
    case EXPR_ADD: return value_eval_add(l, r);
    case EXPR_SUB: return value_eval_sub(l, r);
    case EXPR_MUL: return value_eval_mul(l, r);
    case EXPR_DIV: return value_eval_div(l, r);
    case EXPR_MOD: return value_eval_mod(l, r);
    case EXPR_ABS: return value_eval_abs(l);
    case EXPR_LIT_FLOAT: return value_(e.float_value);
    case EXPR_LIT_INT: return value_(e.int_value);
    case EXPR_LIT_BOOL: return value_(e.bool_value);
    case EXPR_PARAM: return value_(anim_inst->getParamValue(e.param_index));
    case EXPR_EVENT: return value_(false); // TODO
    case EXPR_SIGNAL: return value_(anim_inst->isSignalTriggered(e.signal_index));
    case EXPR_CALL_FEEDBACK_EVENT: anim_inst->triggerFeedbackEvent(e.feedback_event_index); return value_(true);
    default: assert(false);
    }
    return value_(false);
}
value_ value_eval_state_transition(animAnimatorInstance* anim_inst, animFsmState* state, const expr_& e, value_& l, value_& r) {
    switch (e.type) {
    case EXPR_TRANS_IS_STATE_DONE: return value_(state->isAnimFinished(anim_inst));
    }

    return value_eval(anim_inst, e, l, r);
}
value_ expr_::evaluate(animAnimatorInstance* anim_inst) const {
    value_ l;
    value_ r;
    if (left) l = left->evaluate(anim_inst);
    if (right) r = right->evaluate(anim_inst);

    return value_eval(anim_inst, *this, l, r);
}
value_ expr_::evaluate_state_transition(animAnimatorInstance* anim_inst, animFsmState* state) const {
    value_ l, r;
    if (left) l = left->evaluate_state_transition(anim_inst, state);
    if (right) r = right->evaluate_state_transition(anim_inst, state);

    return value_eval_state_transition(anim_inst, state, *this, l, r);
}
std::string expr_::toString(AnimatorEd* animator) const {
    switch (type) {
    case EXPR_NOOP:
        return "noop";
    case EXPR_EQ:
        return MKSTR("(" << left->toString(animator) << " == " << right->toString(animator) << ")");
        break;
    case EXPR_NOTEQ:
        return MKSTR("(" << left->toString(animator) << " != " << right->toString(animator) << ")");
        break;
    case EXPR_LESS:
        return MKSTR("(" << left->toString(animator) << " < " << right->toString(animator) << ")");
        break;
    case EXPR_MORE:
        return MKSTR("(" << left->toString(animator) << " > " << right->toString(animator) << ")");
        break;
    case EXPR_LESSEQ:
        return MKSTR("(" << left->toString(animator) << " <= " << right->toString(animator) << ")");
        break;
    case EXPR_MOREEQ:
        return MKSTR("(" << left->toString(animator) << " >= " << right->toString(animator) << ")");
        break;
    case EXPR_ADD:
        return MKSTR("(" << left->toString(animator) << " + " << right->toString(animator) << ")");
        break;
    case EXPR_SUB:
        return MKSTR("(" << left->toString(animator) << " - " << right->toString(animator) << ")");
        break;
    case EXPR_MUL:
        return MKSTR("(" << left->toString(animator) << " * " << right->toString(animator) << ")");
        break;
    case EXPR_DIV:
        return MKSTR("(" << left->toString(animator) << " / " << right->toString(animator) << ")");
        break;
    case EXPR_MOD:
        return MKSTR("(" << left->toString(animator) << " % " << right->toString(animator) << ")");
        break;
    case EXPR_ABS:
        return MKSTR("abs(" << left->toString(animator) << ")");
        break;
    case EXPR_LIT_FLOAT:
        return MKSTR(float_value);
        break;
    case EXPR_LIT_INT:
        return MKSTR(int_value);
        break;
    case EXPR_LIT_BOOL:
        return MKSTR(bool_value);
        break;
    case EXPR_TRANS_IS_STATE_DONE:
        return "state_complete";
    case EXPR_PARAM:
        return "param";//MKSTR(animator->getParamValue(param_index));
        break;
    case EXPR_EVENT:
        return "event";
    case EXPR_SIGNAL:
        return "signal";
    case EXPR_CALL_FEEDBACK_EVENT:
        return "call_feedback_event";
    default:
        assert(false);
    }
    return "{UNKNOWN}" ;
}