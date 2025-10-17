#pragma once

#include "eval_result.hpp"
#include "target_model.hpp"
#include "conversion.hpp"


inline bool eval_promote_arithmetic(eval_value& v) {
    switch (v.type) {
    case e_bool:
    case e_char:
    case e_signed_char:
    case e_short_int:
        v.type = e_int;
        break;
    case e_unsigned_char:
    case e_unsigned_short_int:
        v.type = e_unsigned_int;
        break;
    }
    return true;
}

inline bool eval_convert_scalar_to_integral(e_fundamental_type target_type, eval_value* v, const target_model& model) {
    const int tgt_nbits = get_bit_size(target_type, model);
    const int src_nbits = get_bit_size(v->type, model);
    const int nbits = std::min(tgt_nbits, src_nbits);
    const uint64_t mask = eval_mask_for_nbits(nbits);

    if (v->is_integral()) {
        if (is_signed(target_type)) {
            v->type = target_type;
            v->i64 = eval_sign_extend(v->i64 & mask, nbits);
        } else {
            v->type = target_type;
            v->u64 = v->u64 & mask;
        }
    } else if (v->is_floating()) {
        v->type = target_type;
        v->i64 = v->f64;
    } else {
        return false;
    }
    return true;
}
inline bool eval_convert_scalar_to_floating(e_fundamental_type target_type, eval_value* v, const target_model& model) {
    const int nbits = get_bit_size(target_type, model);

    if (v->is_integral()) {
        if (v->is_signed()) {
            v->type = target_type;
            v->f64 = eval_truncate_float(v->i64, nbits);
        } else {
            v->type = target_type;
            v->f64 = eval_truncate_float(v->u64, nbits);
        }
    } else if (v->is_floating()) {
        v->type = target_type;
        v->f64 = eval_truncate_float(v->f64, nbits);
    } else {
        return false;
    }
    return true;
}

inline bool eval_convert_promote(e_fundamental_type target_type, eval_value* v) {
    assert(v->type <= target_type);
    if (v->type > target_type) {
        // Implemetation error
        assert(false);
        return false;
    }
    if (v->type == target_type) {
        return true;
    }

    // TODO:
    const target_model& model = target_msvc_win64;

    switch (target_type) {
    case e_void:
        return false;
    case e_nullptr:
        return false;

    case e_bool:
        v->type = e_bool;
        v->u64 = v->u64 ? 1 : 0;
        break;
    case e_char:
    case e_signed_char:
    case e_unsigned_char:
    case e_wchar_t:
    case e_char16_t:
    case e_char32_t:
    case e_short_int:
    case e_unsigned_short_int:
    case e_int:
    case e_unsigned_int:
    case e_long_int:
    case e_unsigned_long_int:
    case e_long_long_int:
    case e_unsigned_long_long_int:
        return eval_convert_scalar_to_integral(target_type, v, model);
    case e_float:
    case e_double:
    case e_long_double:
        return eval_convert_scalar_to_floating(target_type, v, model);
    default:
        return false;
    }
    return true;
}

inline bool eval_is_arithmetic(e_fundamental_type t) {
    switch (t) {
    case e_int: return true;
    case e_unsigned_int: return true;
    case e_long_int: return true;
    case e_unsigned_long_int: return true;
    case e_long_long_int: return true;
    case e_unsigned_long_long_int: return true;

    case e_float: return true;
    case e_double: return true;
    case e_long_double: return true;
    default:
        return false;
    };
}

inline void eval_convert_sort(eval_value*& a, eval_value*& b) {
    if (a->type < b->type) {
        std::swap(a, b);
    }
}

inline bool eval_convert_operands(eval_value& l, eval_value& r) {
    eval_value* a = &l;
    eval_value* b = &r;
    eval_convert_sort(a, b);
    if (!eval_convert_promote(a->type, b)) {
        return false;
    }
    return true;
}

inline eval_result eval_negate_int(const eval_value& v, const target_model& model) {
    const int nbits = get_bit_size(v.type, model);
    const uint64_t mask = eval_mask_for_nbits(nbits);

    eval_value result;
    result.type = v.type;

    if (is_signed(v.type)) {
        int64_t neg = (-v.i64) & mask;
        result.i64 = eval_sign_extend(neg, nbits) & mask;
    } else {
        result.u64 = (-v.u64) & mask;
    }
    return result;
}

inline eval_result eval_negate_float(const eval_value& v, const target_model& model) {
    eval_value result;
    result.type = v.type;
    result.f64 = -v.f64;
    return result;
}

inline eval_result eval_negate(const eval_value& v, const target_model& model = target_msvc_win64) {
    if (v.is_arithmetic_integral()) {
        return eval_negate_int(v, model);
    } else if (v.is_floating()) {
        return eval_negate_float(v, model);
    } else {
        return eval_result::error();
    }
}

inline eval_result eval_logical_not(const eval_value& v, const target_model& model = target_msvc_win64) {
    int is_true = v.u64 == 0 ? 1 : 0;
    eval_value result;
    result.type = e_bool;
    result.u64 = is_true;
    return result;
}

inline eval_result eval_not(const eval_value& v, const target_model& model = target_msvc_win64) {
    if (!v.is_integral()) {
        return eval_result::error();
    }
    eval_value result;
    result.type = v.type;
    result.u64 = ~v.u64;// & eval_mask_for_nbits(get_bit_size(v.type, model));
    eval_mask_integral(result, model);
    return result;
}

inline eval_result eval_multiply(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_arithmetic()) {
        return eval_result::error();
    }

    eval_value result;
    result.type = l.type;

    if (l.is_integral()) {
        const int nbits = get_bit_size(l.type, model);
        const uint64_t mask = eval_mask_for_nbits(nbits);
        if (is_signed(l.type)) {
            int64_t res = (l.i64 * r.i64) & mask;
            result.i64 = eval_sign_extend(res, nbits);
        } else {
            uint64_t res = (l.u64 * r.u64) & mask;
            result.u64 = res;
        }
    } else if (l.is_floating()) {
        result.f64 = l.f64 * r.f64;
    } else {
        return eval_result::error();
    }

    return result;
}
inline eval_result eval_divide(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_arithmetic()) {
        return eval_result::error();
    }

    eval_value result;
    result.type = l.type;

    if (l.is_integral()) {
        if (r.u64 == 0) {
            return eval_result::error();
        }
        const int nbits = get_bit_size(l.type, model);
        const uint64_t mask = eval_mask_for_nbits(nbits);

        if (is_signed(l.type)) {
            int64_t res = (l.i64 / r.i64) & mask;
            result.i64 = eval_sign_extend(res, nbits);
        } else {
            uint64_t res = (l.u64 / r.u64) & mask;
            result.u64 = res;
        }
    } else if (l.is_floating()) {
        result.f64 = l.f64 / r.f64;
    } else {
        return eval_result::error();
    }

    return result;
}
inline eval_result eval_modulo(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_integral()) {
        return eval_result::error();
    }

    const int nbits = get_bit_size(l.type, model);
    const uint64_t mask = eval_mask_for_nbits(nbits);

    eval_value result;
    result.type = l.type;

    if (r.u64 == 0) {
        return eval_result::error();
    }

    if (is_signed(l.type)) {
        int64_t res = (l.i64 % r.i64) & mask;
        result.i64 = eval_sign_extend(res, nbits);
    } else {
        uint64_t res = l.u64 % r.u64;
        result.u64 = res;
    }

    return result;
}

inline eval_result eval_add(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_arithmetic()) {
        return eval_result::error();
    }
    eval_value result;
    result.type = l.type;
    if (l.is_integral()) {
        int nbits = get_bit_size(l.type, model);

        if (is_signed(l.type)) {
            result.i64 = eval_sign_extend((l.i64 + r.i64) & eval_mask_for_nbits(nbits), nbits);
        } else {
            result.u64 = (l.u64 + r.u64) & eval_mask_for_nbits(nbits);
        }
        eval_mask_integral(result, model);
    } else if (l.is_floating()) {
        int nbits = get_bit_size(l.type, model);
        result.f64 = eval_truncate_float(l.f64 + r.f64, nbits);
    } else {
        assert(false);
        return eval_value();
    }
    return result;
}

inline eval_result eval_sub(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_arithmetic()) {
        return eval_result::error();
    }

    eval_value result;
    result.type = l.type;
    if (l.is_integral()) {
        const int nbits = get_bit_size(l.type, model);

        if (is_signed(l.type)) {
            result.i64 = eval_sign_extend((l.i64 - r.i64) & eval_mask_for_nbits(nbits), nbits);
        } else {
            result.u64 = (l.u64 - r.u64) & eval_mask_for_nbits(nbits);
        }
        eval_mask_integral(result, model);
    } else if (l.is_floating()) {
        int nbits = get_bit_size(l.type, model);
        result.f64 = eval_truncate_float(l.f64 - r.f64, nbits);
    } else {
        assert(false);
        return eval_value();
    }
    return result;
}

inline eval_result eval_lshift(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_integral() || !r.is_integral()) {
        return eval_result::error();
    }

    eval_value result;
    result.type = l.type;
    result.u64 = l.u64 << r.u64;
    eval_mask_integral(result, model);
    return result;
}
inline eval_result eval_rshift(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_integral() || !r.is_integral()) {
        return eval_result::error();
    }

    eval_value result;
    result.type = l.type;
    result.u64 = l.u64 >> r.u64;
    eval_mask_integral(result, model);
    return result;
}

inline eval_result eval_less(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_arithmetic()) {
        return eval_result::error();
    }
    if (l.is_integral()) {
        if (l.is_signed()) {
            return eval_value::make_bool(l.i64 < r.i64);
        } else {
            return eval_value::make_bool(l.u64 < r.u64);
        }
    } else {
        return eval_value::make_bool(l.f64 < r.f64);
    }
}
inline eval_result eval_greater(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_arithmetic()) {
        return eval_result::error();
    }
    if (l.is_integral()) {
        if (l.is_signed()) {
            return eval_value::make_bool(l.i64 > r.i64);
        } else {
            return eval_value::make_bool(l.u64 > r.u64);
        }
    } else {
        return eval_value::make_bool(l.f64 > r.f64);
    }
}
inline eval_result eval_less_equal(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_arithmetic()) {
        return eval_result::error();
    }
    if (l.is_integral()) {
        if (l.is_signed()) {
            return eval_value::make_bool(l.i64 <= r.i64);
        } else {
            return eval_value::make_bool(l.u64 <= r.u64);
        }
    } else {
        return eval_value::make_bool(l.f64 <= r.f64);
    }
}
inline eval_result eval_greater_equal(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_arithmetic()) {
        return eval_result::error();
    }
    if (l.is_integral()) {
        if (l.is_signed()) {
            return eval_value::make_bool(l.i64 >= r.i64);
        } else {
            return eval_value::make_bool(l.u64 >= r.u64);
        }
    } else {
        return eval_value::make_bool(l.f64 >= r.f64);
    }
}

inline eval_result eval_equal(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    return eval_value::make_bool(l.u64 == r.u64);
}
inline eval_result eval_not_equal(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    return eval_value::make_bool(l.u64 != r.u64);
}

inline eval_result eval_and(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_integral() || !r.is_integral()) {
        return eval_result::error();
    }

    eval_value result;
    result.type = l.type;
    result.u64 = l.u64 & r.u64;
    eval_mask_integral(result, model);
    return result;
}

inline eval_result eval_xor(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_integral() || !r.is_integral()) {
        return eval_result::error();
    }

    eval_value result;
    result.type = l.type;
    result.u64 = l.u64 ^ r.u64;
    eval_mask_integral(result, model);
    return result;
}

inline eval_result eval_inclusive_or(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    if (!l.is_integral() || !r.is_integral()) {
        return eval_result::error();
    }

    eval_value result;
    result.type = l.type;
    result.u64 = l.u64 | r.u64;
    eval_mask_integral(result, model);
    return result;
}

inline eval_result eval_logical_and(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    return eval_value::make_bool(l.u64 && r.u64);
}

inline eval_result eval_logical_or(const eval_value& l, const eval_value& r, const target_model& model = target_msvc_win64) {
    return eval_value::make_bool(l.u64 || r.u64);
}