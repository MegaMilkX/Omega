#pragma once

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

