#pragma once

#include "eval_value.hpp"
#include "dbg.hpp"


enum e_eval_result {
    e_eval_ok = 0,
    e_eval_not_implemented,
    e_eval_not_a_constant_expr,
    e_eval_error
};
inline const char* eval_result_to_string(e_eval_result e) {
    switch (e) {
    case e_eval_ok: return "e_eval_ok";
    case e_eval_not_implemented: return "e_eval_not_implemented";
    case e_eval_not_a_constant_expr: return "e_eval_not_a_constant_expr";
    case e_eval_error: return "e_eval_error";
    default: return "[UNKNOWN]";
    }
}

struct eval_result {
    e_eval_result result;
    eval_value value;

    eval_result(e_eval_result r)
        : result(r) {}
    eval_result(eval_value val)
        : result(e_eval_ok), value(val) {}

    operator bool() const {
        return result == e_eval_ok;
    }

    static eval_result not_implemented() {
        assert(false);
        return eval_result(e_eval_not_implemented);
    }
    static eval_result not_a_constant_expr() {
        assert(false);
        return eval_result(e_eval_not_a_constant_expr);
    }
    static eval_result error() {
        assert(false);
        return eval_result(e_eval_error);
    }

    void dbg_print() {
        dbg_printf_color("%s", DBG_WHITE, eval_result_to_string(result));
        if (result == e_eval_ok) {
            dbg_printf_color(": ", DBG_WHITE);
            value.dbg_print();
        }
    }
};

