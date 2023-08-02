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
    case PP_NUMBER: tok.type = tt_literal; break;
    case CHARACTER_LITERAL: tok.type = tt_literal; break;
    case USER_DEFINED_CHARACTER_LITERAL: tok.type = tt_literal; break;
    case STRING_LITERAL: tok.type = tt_literal; break;
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
