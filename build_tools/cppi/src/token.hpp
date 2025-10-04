#pragma once

#include <assert.h>
#include <string>
#include "preprocessor/pp_file.hpp"


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
enum token_literal_type {
    l_none,
    l_int32,
    l_int64,
    l_uint32,
    l_uint64,
    l_float,
    l_double,
    l_long_double,
    l_char,
    l_char16,
    l_char32,
    l_wchar,
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
inline const char* keyword_to_string(e_keyword kw) {
    switch (kw) {
    case kw_alignas: return "alignas";
    case kw_continue: return "continue";
    case kw_friend: return "friend";
    case kw_register: return "register";
    case kw_true: return "true";
    case kw_alignof: return "alignof";
    case kw_decltype: return "decltype";
    case kw_goto: return "goto";
    case kw_reinterpret_cast: return "reinterpret_cast";
    case kw_try: return "try";
    case kw_asm: return "asm";
    case kw_default: return "default";
    case kw_if: return "if";
    case kw_return: return "return";
    case kw_typedef: return "typedef";
    case kw_auto: return "auto";
    case kw_delete: return "delete";
    case kw_inline: return "inline";
    case kw_short: return "short";
    case kw_typeid: return "typeid";
    case kw_bool: return "bool";
    case kw_do: return "do";
    case kw_int: return "int";
    case kw_signed: return "signed";
    case kw_typename: return "typename";
    case kw_break: return "break";
    case kw_double: return "double";
    case kw_long: return "long";
    case kw_sizeof: return "sizeof";
    case kw_union: return "union";
    case kw_case: return "case";
    case kw_dynamic_cast: return "dynamic_cast";
    case kw_mutable: return "mutable";
    case kw_static: return "static";
    case kw_unsigned: return "unsigned";
    case kw_catch: return "catch";
    case kw_else: return "else"; 
    case kw_namespace: return "namespace"; 
    case kw_static_assert: return "static_assert";
    case kw_using: return "using";
    case kw_char: return "char";
    case kw_enum: return "enum"; 
    case kw_new: return "new"; 
    case kw_static_cast: return "static_cast";
    case kw_virtual: return "virtual";
    case kw_char16_t: return "char16_t";
    case kw_explicit: return "explicit"; 
    case kw_noexcept: return "noexcept"; 
    case kw_struct: return "struct";
    case kw_void: return "void";
    case kw_char32_t: return "char32_t";
    case kw_export: return "export"; 
    case kw_nullptr: return "nullptr"; 
    case kw_switch: return "switch";
    case kw_volatile: return "volatile";
    case kw_class: return "class";
    case kw_extern: return "extern"; 
    case kw_operator: return "operator"; 
    case kw_template: return "template";
    case kw_wchar_t: return "wchar_t";
    case kw_const: return "const";
    case kw_false: return "false"; 
    case kw_private: return "private"; 
    case kw_this: return "this";
    case kw_while: return "while";
    case kw_constexpr: return "constexpr";
    case kw_float: return "float"; 
    case kw_protected: return "protected"; 
    case kw_thread_local: return "thread_local";
    case kw_const_cast: return "const_cast";
    case kw_for: return "for"; 
    case kw_public: return "public"; 
    case kw_throw: return "throw";
    default:
        assert(false);
        return "UNKNOWN";
    };
}
struct token {
    const PP_FILE* file = 0;
    token_type type = tt_unk;
    std::string str;
    std::string strlit_content;
    int line = 0;
    int col = 0;

    union {
        struct {
            e_keyword kw_type;
        };
        struct {
            token_literal_type lit_type;
            union {
                int8_t i8;
                int16_t i16;
                int32_t i32;
                int64_t i64;
                uint8_t ui8;
                uint16_t ui16;
                uint32_t ui32;
                uint64_t ui64;
                float f32;
                double double_;
                long double long_double_;
            };
        };
    };
    bool transient;
};
