#pragma once

#include <string>
#include "pp_file.hpp"


enum token_type {
    tt_identifier,
    tt_keyword,
    tt_literal,
    tt_string_literal,
    tt_operator,
    tt_punctuator,
    tt_unk,
    tt_eof
};
enum e_keyword {
    kw_alignas, kw_continue, kw_friend, kw_register, kw_true
    , kw_alignof, kw_decltype, kw_goto, kw_reinterpret_cast, kw_try
    , kw_asm, kw_default, kw_if, kw_return, kw_typedef
    , kw_auto, kw_delete, kw_inline, kw_short, kw_typeid
    , kw_bool, kw_do, kw_int, kw_signed, kw_typename
    , kw_break, kw_double, kw_long, kw_sizeof, kw_union
    , kw_case, kw_dynamic_cast, kw_mutable, kw_static, kw_unsigned
    , kw_catch, kw_else, kw_namespace, kw_static_assert, kw_using
    , kw_char, kw_enum, kw_new, kw_static_cast, kw_virtual
    , kw_char16_t, kw_explicit, kw_noexcept, kw_struct, kw_void
    , kw_char32_t, kw_export, kw_nullptr, kw_switch, kw_volatile
    , kw_class, kw_extern, kw_operator, kw_template, kw_wchar_t
    , kw_const, kw_false, kw_private, kw_this, kw_while
    , kw_constexpr, kw_float, kw_protected, kw_thread_local
    , kw_const_cast, kw_for, kw_public, kw_throw
};
struct token {
    PP_FILE* file = 0;
    token_type type = tt_unk;
    std::string str;
    std::string strlit_content;
    int line = 0;
    int col = 0;
    union {
        e_keyword kw_type;
    };
    bool transient;
};
