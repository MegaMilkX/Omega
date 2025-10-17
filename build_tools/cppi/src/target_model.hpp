#pragma once

#include "types.hpp"


struct target_model {
    int bits_pointer = 64;

    int bits_char = 8;
    int bits_short = 16;
    int bits_int = 32;
    int bits_long = 32;
    int bits_long_long = 64;

    int bits_float = 32;
    int bits_double = 64;
    int bits_long_double = 64;

    int bits_wchar = 16;
};

constexpr target_model target_msvc_win64 = {
    .bits_pointer = 64,

    .bits_char = 8,
    .bits_short = 16,
    .bits_int = 32,
    .bits_long = 32,
    .bits_long_long = 64,

    .bits_float = 32,
    .bits_double = 64,
    .bits_long_double = 64,

    .bits_wchar = 16,
};

inline int get_bit_size(e_fundamental_type t, const target_model& mdl) {
    switch (t) {
    case e_fundamental_invalid:
        return 0;

    case e_void:
        return 0;
    case e_nullptr:
        return mdl.bits_pointer;
    
    case e_bool:
        return 8;
    case e_char:
    case e_signed_char:
    case e_unsigned_char:
        return mdl.bits_char;
    case e_wchar_t:
        return mdl.bits_wchar;
    case e_char16_t:
        return 16;
    case e_char32_t:
        return 32;

    case e_short_int:
    case e_unsigned_short_int:
        return mdl.bits_short;
    case e_int:
    case e_unsigned_int:
        return mdl.bits_int;
    case e_long_int:
    case e_unsigned_long_int:
        return mdl.bits_long;
    case e_long_long_int:
    case e_unsigned_long_long_int:
        return mdl.bits_long_long;

    case e_float:
        return mdl.bits_float;
    case e_double:
        return mdl.bits_double;
    case e_long_double:
        return mdl.bits_long_double;
    
    default:
        return 0;
    };
}

