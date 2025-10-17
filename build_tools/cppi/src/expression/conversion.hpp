#pragma once

#include "eval_result.hpp"
#include "target_model.hpp"


inline eval_result eval_promote_integral(const eval_value& v, const target_model& model) {
    if (!v.is_integral()) {
        return eval_result::error();
    }

    switch (v.type) {
    case e_bool:
    case e_char:
    case e_signed_char:
    case e_unsigned_char:
    case e_short_int:
    case e_unsigned_short_int:
        if (v.is_signed()) {
            return eval_value(e_int, v.i64, model);
        } else {
            if (v.u64 > powl(2, get_bit_size(e_int, model) - 1) - 1) {
                return eval_value(e_unsigned_int, v.u64, model);
            } else {
                return eval_value(e_int, v.i64, model);
            }
        }
    default:
        return v;
    }
}

inline void eval_mask_integral(eval_value& v, const target_model& model) {
    const int nbits = get_bit_size(v.type, model);
    const uint64_t mask = eval_mask_for_nbits(nbits);
    v.u64 = v.u64 & mask;
}