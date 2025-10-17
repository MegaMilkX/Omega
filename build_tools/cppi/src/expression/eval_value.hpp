#pragma once

#include <stdint.h>
#include <string>
#include "dbg.hpp"
#include "types.hpp"
#include "target_model.hpp"


inline uint64_t eval_mask_for_nbits(int nbits) {
    return nbits >= 64 ? ~uint64_t(0) : ((uint64_t(1) << nbits) - 1);
}
inline int64_t eval_sign_extend(uint64_t value, int nbits) {
    if(nbits >= 64) return (int64_t)value;
    uint64_t mask = uint64_t(1) << (nbits - 1);
    return (int64_t)((value ^ mask) - mask);
}

inline long double eval_truncate_float(long double value, int nbits) {
    switch (nbits) {
    case 32: return static_cast<float>(value);
    case 64: return static_cast<double>(value);
    default:
        return value;
    }
}

struct eval_value {
    e_fundamental_type type;

    union {
        uint64_t u64;
        int64_t  i64;
        long double f64;
    };
    

    eval_value()
        : type(e_fundamental_invalid) {}

    explicit eval_value(e_fundamental_type t, int64_t i64, const target_model& model = target_msvc_win64)
        : type(t) {
        const int nbits = get_bit_size(t, model);
        const uint64_t mask = eval_mask_for_nbits(nbits);
        this->i64 = eval_sign_extend(i64 & mask, nbits);
    }
    explicit eval_value(e_fundamental_type t, uint64_t u64, const target_model& model = target_msvc_win64)
        : type(t) {
        const int nbits = get_bit_size(t, model);
        const uint64_t mask = eval_mask_for_nbits(nbits);
        this->u64 = u64 & mask;
    }
    explicit eval_value(e_fundamental_type t, long double f64, const target_model& model = target_msvc_win64)
        : type(t) {
        const int nbits = get_bit_size(t, model);
        this->f64 = eval_truncate_float(f64, nbits);    
    }
    explicit eval_value(std::nullptr_t n)
        : type(e_nullptr) {}

    static eval_value make_bool(bool v) {
        return eval_value(e_bool, (uint64_t)(v ? 1 : 0));
    }

    bool is_empty() const {
        // TODO: handle non-fundamentals
        return type == e_fundamental_invalid;
    }

    bool is_floating() const {
        return type == e_float || type == e_double || type == e_long_double;
    }
    bool is_arithmetic() const {
        return 
            type == e_int ||
            type == e_unsigned_int ||
            type == e_long_int ||
            type == e_unsigned_long_int ||
            type == e_long_long_int ||
            type == e_unsigned_long_long_int ||
            type == e_float ||
            type == e_double ||
            type == e_long_double;
    }
    bool is_arithmetic_integral() const {
        return 
            type == e_int ||
            type == e_unsigned_int ||
            type == e_long_int ||
            type == e_unsigned_long_int ||
            type == e_long_long_int ||
            type == e_unsigned_long_long_int;
    }
    bool is_integral() const {
        return
            type == e_bool ||
            type == e_char ||
            type == e_signed_char ||
            type == e_unsigned_char ||
            type == e_wchar_t ||
            type == e_char16_t ||
            type == e_char32_t ||
            type == e_short_int ||
            type == e_unsigned_short_int ||
            type == e_int ||
            type == e_unsigned_int ||
            type == e_long_int ||
            type == e_unsigned_long_int ||
            type == e_long_long_int ||
            type == e_unsigned_long_long_int;
    }
    bool is_signed() const {
        return ::is_signed(type);
    }

    std::string normalized() const {
        switch (type) {
        case e_fundamental_invalid: return "[INVALID]";

        case e_void: return "[VOID]";
        case e_nullptr: return "nullptr";
    
        case e_bool: return u64 ? "1" : "0";
        case e_char: return std::to_string((char)i64);
        case e_signed_char: return std::to_string((signed char)i64);
        case e_unsigned_char: return std::to_string((unsigned char)u64);
        case e_wchar_t: return std::to_string((wchar_t)i64);
        case e_char16_t: return std::to_string((char16_t)i64);
        case e_char32_t: return std::to_string((char32_t)i64);

        case e_short_int: return std::to_string((short int)i64);
        case e_unsigned_short_int: return std::to_string((unsigned short int)i64);
        case e_int: return std::to_string((int)i64);
        case e_unsigned_int: return std::to_string((unsigned int)u64);
        case e_long_int: return std::to_string((long int)i64);
        case e_unsigned_long_int: return std::to_string((unsigned long int)u64);
        case e_long_long_int: return std::to_string((long long int)i64);
        case e_unsigned_long_long_int: return std::to_string((unsigned long long int)u64);

        case e_float: return std::to_string((float)f64);
        case e_double: return std::to_string((double)f64);
        case e_long_double: return std::to_string((long double)f64);
        default:
            return "[not_implemented]";
        }
    }

    void dbg_print() const {
        if (type == e_bool) {
            dbg_printf_color("%s", DBG_KEYWORD, u64 ? "true" : "false");
            return;
        }
        dbg_printf_color("%s", DBG_WHITE, normalized().c_str());
    }
};

