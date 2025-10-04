#include "parse_state.hpp"


static const char* keywords[] = { 
    "alignas", "continue", "friend", "register", "true",
    "alignof", "decltype", "goto", "reinterpret_cast", "try",
    "asm", "default", "if", "return", "typedef",
    "auto", "delete", "inline", "short", "typeid",
    "bool", "do", "int", "signed", "typename",
    "break", "double", "long", "sizeof", "union",
    "case", "dynamic_cast", "mutable", "static", "unsigned",
    "catch", "else", "namespace", "static_assert", "using",
    "char", "enum", "new", "static_cast", "virtual",
    "char16_t", "explicit", "noexcept", "struct", "void",
    "char32_t", "export", "nullptr", "switch", "volatile",
    "class", "extern", "operator", "template", "wchar_t",
    "const", "false", "private", "this", "while",
    "constexpr", "float", "protected", "thread_local",
    "const_cast", "for", "public", "throw" 
};

void pp_number_to_token(PP_TOKEN& pp_tok, token& tok) {
    switch (pp_tok.number_type) {
    case PP_NUMBER_INTEGRAL:
        switch (pp_tok.integral_type) {
        case PP_INT32: tok.lit_type = l_int32; tok.i32 = pp_tok.i32; break;
        case PP_INT64: tok.lit_type = l_int64; tok.i64 = pp_tok.i64; break;
        case PP_UINT32: tok.lit_type = l_uint32; tok.ui32 = pp_tok.ui32; break;
        case PP_UINT64: tok.lit_type = l_uint64; tok.ui64 = pp_tok.ui64; break;
        default:
            assert(false);
            tok.lit_type = l_none;
        }
        break;
    case PP_NUMBER_FLOATING:
        switch (pp_tok.floating_type) {
        case PP_FLOAT32: tok.lit_type = l_float; tok.f32 = pp_tok.float_; break;
        case PP_FLOAT64: tok.lit_type = l_double; tok.double_ = pp_tok.double_; break;
        case PP_LONG_DOUBLE: tok.lit_type = l_long_double; tok.long_double_ = pp_tok.long_double_; break;
        default:
            assert(false);
            tok.lit_type = l_none;
        }
        break;
    default:
        assert(false);
        tok.lit_type = l_none;
    }
}

token convert_pp_token(PP_TOKEN& pp_tok) {
    token tok;
    switch (pp_tok.type) {
    case IDENTIFIER:
        tok.type = tt_identifier;
        for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
            if (pp_tok.str == keywords[i]) {
                tok.type = tt_keyword;
                tok.kw_type = (e_keyword)i;
                break;
            }
        }
        break;
    case PP_NUMBER:
        tok.type = tt_literal;
        pp_number_to_token(pp_tok, tok);
        break;
    case CHARACTER_LITERAL: tok.type = tt_literal; break;
    case USER_DEFINED_CHARACTER_LITERAL: tok.type = tt_literal; break;
    case STRING_LITERAL:
        tok.type = tt_string_literal;
        tok.strlit_content = pp_tok.strlit_content;
        break;
    case USER_DEFINED_STRING_LITERAL: tok.type = tt_literal; break;
    case PP_OP_OR_PUNC: tok.type = tt_punctuator; break;
    case END_OF_FILE: tok.type = tt_eof; break;
    default: tok.type = tt_unk; break;
    }
    tok.file = pp_tok.file;
    tok.str = pp_tok.str;
    tok.line = pp_tok.line;
    tok.col = pp_tok.col;
    tok.transient = false;
    return tok;
}
