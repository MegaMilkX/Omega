#pragma once

#include <assert.h>

/*
enum e_built_in_type {
    e_empty,
    e_bool,
    e_null,
    e_int8,         // char
    e_uint8,        // unsigned char
    e_int16,        // short
    e_uint16,       // unsigned short
    e_int32,        // int
    e_uint32,       // unsigned int, unsigned
    e_int64,        // long
    e_uint64,       // unsigned long
    e_float,
    e_double,
    e_long_double,
    e_string // TODO: REMOVE THIS
};*/

// This enum encodes rank, do not reorder
enum e_fundamental_type {
    e_fundamental_invalid,

    e_void,
    e_nullptr,
    
    e_bool,
    e_char,
    e_signed_char,
    e_unsigned_char,
    e_wchar_t,
    e_char16_t,
    e_char32_t,

    e_short_int,
    e_unsigned_short_int,
    e_int,
    e_unsigned_int,
    e_long_int,
    e_unsigned_long_int,
    e_long_long_int,
    e_unsigned_long_long_int,

    e_float,
    e_double,
    e_long_double,
};
inline bool is_signed(e_fundamental_type t) {
    if(t == e_unsigned_char
        || t == e_unsigned_short_int
        || t == e_unsigned_int
        || t == e_unsigned_long_int
        || t == e_unsigned_long_long_int
    ) {
        return false;
    }
    return true;
}
inline bool is_floating(e_fundamental_type t) {
    return t == e_float || t == e_double || t == e_long_double;
}
inline const char* fundamental_type_to_string(e_fundamental_type t) {
    switch(t) {
    case e_fundamental_invalid: return "[INVALID]";

    case e_void: return "void";
    case e_nullptr: return "nullptr";

    case e_bool: return "bool";
    case e_char: return "char";
    case e_signed_char: return "signed char";
    case e_unsigned_char: return "unsigned char";
    case e_wchar_t: return "wchar_t";
    case e_char16_t: return "char16_t";
    case e_char32_t: return "char32_t";

    case e_short_int: return "short int";
    case e_unsigned_short_int: return "unsigned short int";
    case e_int: return "int";
    case e_unsigned_int: return "unsigned int";
    case e_long_int: return "long int";
    case e_unsigned_long_int: return "unsigned long int";
    case e_long_long_int: return "long long int";
    case e_unsigned_long_long_int: return "unsigned long long int";

    case e_float: return "float";
    case e_double: return "double";
    case e_long_double: return "long double";

    default:
        assert(false);
        return "[UNKNOWN]";
    };
}
inline const char* fundamental_type_to_normalized_string(e_fundamental_type t) {
    switch(t) {
    //case e_fundamental_invalid: return "";

    case e_void: return "v";
    case e_nullptr: return "Dn";

    case e_bool: return "b";
    case e_char: return "c";
    case e_signed_char: return "a";
    case e_unsigned_char: return "h";
    case e_wchar_t: return "w";
    case e_char16_t: return "Ds";
    case e_char32_t: return "Di";

    case e_short_int: return "s";
    case e_unsigned_short_int: return "t";
    case e_int: return "i";
    case e_unsigned_int: return "j";
    case e_long_int: return "l";
    case e_unsigned_long_int: return "m";
    case e_long_long_int: return "x";
    case e_unsigned_long_long_int: return "y";

    case e_float: return "f";
    case e_double: return "d";
    case e_long_double: return "e";

    // Unsure which style to adopt yet
    /*
    case e_void: return "v";
    case e_nullptr: return "n";
    
    case e_bool: return "b";
    case e_char: return "c";
    case e_signed_char: return "sc";
    case e_unsigned_char: return "uc";
    case e_wchar_t: return "wc";
    case e_char16_t: return "c16";
    case e_char32_t: return "c32";

    case e_short_int: return "si";
    case e_unsigned_short_int: return "usi";
    case e_int: return "i";
    case e_unsigned_int: return "ui";
    case e_long_int: return "li";
    case e_unsigned_long_int: return "uli";
    case e_long_long_int: return "lli";
    case e_unsigned_long_long_int: return "ulli";

    case e_float: return "f";
    case e_double: return "d";
    case e_long_double: return "ld";
    */
    default:
        assert(false);
        return "";
    };
}

enum e_operator {
    e_operator_none,
    e_add,
    e_sub,
    e_mul,
    e_div,
    e_mod,
    e_lshift,
    e_rshift,
    e_less,
    e_more,
    e_lesseq,
    e_moreeq,
    e_equal,
    e_notequal,
    e_and,
    e_exor,
    e_ior,
    e_land,
    e_lor,
};
inline const char* operator_to_string(e_operator op) {
    switch (op) {
    case e_operator_none: return "[NONE]";
    case e_add: return "+";
    case e_sub: return "-";
    case e_mul: return "*";
    case e_div: return "/";
    case e_mod: return "%";
    case e_lshift: return "<<";
    case e_rshift: return ">>";
    case e_less: return "<";
    case e_more: return ">";
    case e_lesseq: return "<=";
    case e_moreeq: return ">=";
    case e_equal: return "==";
    case e_notequal: return "!=";
    case e_and: return "&";
    case e_exor: return "^";
    case e_ior: return "|";
    case e_land: return "&&";
    case e_lor: return "||";
    default: return "[UNKNOWN]";
    };
}

enum e_access_specifier {
    e_access_unspecified,
    e_access_private,
    e_access_protected,
    e_access_public
};
inline const char* access_to_string(e_access_specifier e) {
    switch (e) {
    case e_access_unspecified: return "unspecified";
    case e_access_private: return "private";
    case e_access_protected: return "protected";
    case e_access_public: return "public";
    default: return "UNKNOWN";
    }
}

enum e_class_key {
    ck_class,
    ck_struct,
    ck_union
};
inline const char* class_key_to_string(e_class_key e) {
    switch (e) {
    case ck_class: return "class";
    case ck_struct: return "struct";
    case ck_union: return "union";
    default: return "UNKNOWN";
    }
}

typedef int e_virt_flags;
enum e_virt {
    e_virt_none = 0x0,
    e_override  = 0x1,
    e_final     = 0x2
};
inline const char* virt_to_string(e_virt e) {
    switch (e) {
    case e_override: return "override";
    case e_final: return "final";
    default: return "UNKNOWN";
    }
}

enum e_enum_key {
    ck_enum,
    ck_enum_class,
    ck_enum_struct
};
inline const char* enum_key_to_string(e_enum_key k) {
    switch (k) {
    case ck_enum: return "enum";
    case ck_enum_class: return "enum class";
    case ck_enum_struct: return "enum struct";
    default: return "";
    }
}

enum e_elaborated_type {
    e_elaborated_class,
    e_elaborated_enum
};

enum e_cv {
    cv_const,
    cv_volatile
};
inline const char* cv_to_string(e_cv e) {
    switch (e) {
    case cv_const: return "const";
    case cv_volatile: return "volatile";
    default: return "UNKNOWN";
    }
}

enum e_ptr_operator {
    e_ptr,
    e_ref,
    e_rvref
};

enum e_unary_op {
    e_unary_deref,
    e_unary_amp,
    e_unary_plus,
    e_unary_minus,
    e_unary_lnot,
    e_unary_bnot,
    e_unary_incr_pre,
    e_unary_incr_post,
    e_unary_decr_pre,
    e_unary_decr_post,
};
inline const char* unary_op_to_string(e_unary_op op) {
    switch (op) {
    case e_unary_deref: return "*";
    case e_unary_amp: return "&";
    case e_unary_plus: return "+";
    case e_unary_minus: return "-";
    case e_unary_lnot: return "!";
    case e_unary_bnot: return "~";
    case e_unary_incr_pre: return "++";
    case e_unary_incr_post: return "++";
    case e_unary_decr_pre: return "--";
    case e_unary_decr_post: return "--";
    }
    return "[e_unary_op unknown]";
}

enum e_cast {
    e_cast_cstyle,
    e_cast_dynamic,
    e_cast_static,
    e_cast_reinterpret,
    e_cast_const,
};

