#pragma once

#include <memory>
#include "value/value.hpp"


class AnimatorEd;
class animAnimatorInstance;
class animFsmState;
enum EXPR_TYPE {
    EXPR_NOOP,
    EXPR_EQ,
    EXPR_NOTEQ,
    EXPR_LESS,
    EXPR_MORE,
    EXPR_LESSEQ,
    EXPR_MOREEQ,

    EXPR_ADD,
    EXPR_SUB,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_MOD,
    EXPR_ABS,

    EXPR_LIT_FLOAT,
    EXPR_LIT_INT,
    EXPR_LIT_BOOL,

    EXPR_TRANS_IS_STATE_DONE,

    EXPR_PARAM,
    EXPR_EVENT,
    EXPR_SIGNAL,

    EXPR_CALL_FEEDBACK_EVENT
};
struct expr_ {
    EXPR_TYPE type;
    std::shared_ptr<expr_> left;
    std::shared_ptr<expr_> right;
    union {
        int     param_index;
        int     signal_index;
        int     feedback_event_index;
        float   float_value;
        int     int_value;
        bool    bool_value;
    };

    expr_() : type(EXPR_NOOP) {}
    expr_(EXPR_TYPE type, expr_* left, expr_* right) : type(type), left(left), right(right) {}
    expr_(float val) : expr_(EXPR_LIT_FLOAT, 0, 0) { float_value = val; }
    expr_(int val) : expr_(EXPR_LIT_INT, 0, 0) { int_value = val; }
    expr_(bool val) : expr_(EXPR_LIT_BOOL, 0, 0) { bool_value = val; }

    value_ evaluate(animAnimatorInstance* anim_inst) const;
    value_ evaluate_state_transition(animAnimatorInstance* anim_inst, animFsmState* state) const;

    std::string toString(AnimatorEd* animator) const;
    void dbgLog(AnimatorEd* animator) const {
        LOG_DBG(toString(animator));
    }
};
inline expr_ operator+  (const expr_& a, const float& b) { return expr_{ EXPR_ADD, new expr_(a), new expr_(b) }; }
inline expr_ operator-  (const expr_& a, const float& b) { return expr_{ EXPR_SUB, new expr_(a), new expr_(b) }; }
inline expr_ operator*  (const expr_& a, const float& b) { return expr_{ EXPR_MUL, new expr_(a), new expr_(b) }; }
inline expr_ operator/  (const expr_& a, const float& b) { return expr_{ EXPR_DIV, new expr_(a), new expr_(b) }; }
inline expr_ operator%  (const expr_& a, const float& b) { return expr_{ EXPR_MOD, new expr_(a), new expr_(b) }; }

inline expr_ operator+  (const float& a, const expr_& b) { return expr_{ EXPR_ADD, new expr_(a), new expr_(b) }; }
inline expr_ operator-  (const float& a, const expr_& b) { return expr_{ EXPR_SUB, new expr_(a), new expr_(b) }; }
inline expr_ operator*  (const float& a, const expr_& b) { return expr_{ EXPR_MUL, new expr_(a), new expr_(b) }; }
inline expr_ operator/  (const float& a, const expr_& b) { return expr_{ EXPR_DIV, new expr_(a), new expr_(b) }; }
inline expr_ operator%  (const float& a, const expr_& b) { return expr_{ EXPR_MOD, new expr_(a), new expr_(b) }; }

inline expr_ operator+  (const expr_& a, const expr_& b) { return expr_{ EXPR_ADD, new expr_(a), new expr_(b) }; }
inline expr_ operator-  (const expr_& a, const expr_& b) { return expr_{ EXPR_SUB, new expr_(a), new expr_(b) }; }
inline expr_ operator*  (const expr_& a, const expr_& b) { return expr_{ EXPR_MUL, new expr_(a), new expr_(b) }; }
inline expr_ operator/  (const expr_& a, const expr_& b) { return expr_{ EXPR_DIV, new expr_(a), new expr_(b) }; }
inline expr_ operator%  (const expr_& a, const expr_& b) { return expr_{ EXPR_MOD, new expr_(a), new expr_(b) }; }

inline expr_ operator== (const expr_& a, const expr_& b) { return expr_{ EXPR_EQ, new expr_(a), new expr_(b) }; }
inline expr_ operator!= (const expr_& a, const expr_& b) { return expr_{ EXPR_NOTEQ, new expr_(a), new expr_(b) }; }
inline expr_ operator<  (const expr_& a, const expr_& b) { return expr_{ EXPR_LESS, new expr_(a), new expr_(b) }; }
inline expr_ operator>  (const expr_& a, const expr_& b) { return expr_{ EXPR_MORE, new expr_(a), new expr_(b) }; }
inline expr_ operator<= (const expr_& a, const expr_& b) { return expr_{ EXPR_LESSEQ, new expr_(a), new expr_(b) }; }
inline expr_ operator>= (const expr_& a, const expr_& b) { return expr_{ EXPR_MOREEQ, new expr_(a), new expr_(b) }; }

struct param_ {
    int index;
    param_(AnimatorEd* animator, const std::string& name);
    operator expr_() const {
        expr_ e(EXPR_PARAM, 0, 0);
        e.param_index = index;
        return e;
    }
};
struct abs_ {
    expr_ e;
    abs_(const expr_& e) : e(e) {}
    operator expr_() const { return expr_(EXPR_ABS, new expr_(e), 0); }
};
struct state_complete_ {
    operator expr_() const { return expr_(EXPR_TRANS_IS_STATE_DONE, 0, 0); }
};
struct signal_ {
    int index;
    signal_(AnimatorEd* aniamtor, const std::string& signal_name);
    operator expr_() const {
        expr_ e(EXPR_SIGNAL, 0, 0);
        e.signal_index = index;
        return e; 
    }
};
struct call_feedback_event_ {
    int index;
    call_feedback_event_(AnimatorEd* animator, const std::string& fb_event_name);
    operator expr_() const {
        expr_ e(EXPR_CALL_FEEDBACK_EVENT, 0, 0);
        e.feedback_event_index = index;
        return e;
    }
};