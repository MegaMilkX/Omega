#pragma once

#include <string>


enum PP_TOKEN_TYPE {
    END_OF_FILE,
    WHITESPACE,
    HEADER_NAME,
    IDENTIFIER,
    PP_NUMBER,
    CHARACTER_LITERAL,
    USER_DEFINED_CHARACTER_LITERAL,
    STRING_LITERAL,
    USER_DEFINED_STRING_LITERAL,
    PP_OP_OR_PUNC,
    PP_ANY,
    NEW_LINE,

    MACRO_PARAM,
    PLACEMARKER
};
enum PP_NUMBER_TYPE {
    PP_NUMBER_NONE,
    PP_NUMBER_INTEGRAL,
    PP_NUMBER_FLOATING
};
enum PP_INTEGRAL_TYPE {
    PP_INT32,
    PP_INT64,
    PP_UINT32,
    PP_UINT64
};
enum PP_FLOATING_TYPE {
    PP_FLOAT32,
    PP_FLOAT64,
    PP_LONG_DOUBLE
};
enum PP_INTEGRAL_POSTFIX {
    PP_INTEGRAL_POSTFIX_NONE,
    PP_INTEGRAL_POSTFIX_USER,
    PP_INTEGRAL_POSTFIX_L,
    PP_INTEGRAL_POSTFIX_LL,
    PP_INTEGRAL_POSTFIX_U,
    PP_INTEGRAL_POSTFIX_UL,
    PP_INTEGRAL_POSTFIX_ULL
};
enum PP_FLOATING_POSTFIX {
    PP_FLOATING_POSTFIX_NONE,
    PP_FLOATING_POSTFIX_USER,
    PP_FLOATING_POSTFIX_F,
    PP_FLOATING_POSTFIX_L,
};
enum PP_NUMBER_LITERAL {
    PP_NUMBER_LITERAL_BINARY,
    PP_NUMBER_LITERAL_OCTAL,
    PP_NUMBER_LITERAL_DECIMAL,
    PP_NUMBER_LITERAL_HEXADECIMAL,
    PP_NUMBER_LITERAL_FLOATING,
};

struct PP_FILE;
struct PP_TOKEN {
    const PP_FILE* file = 0;
    PP_TOKEN_TYPE type;
    std::string str;
    std::string strlit_content;
    int param_idx = -1;
    int line = -1;
    int col = -1;
    bool is_macro_expandable = true;
    PP_NUMBER_TYPE number_type = PP_NUMBER_NONE;

    PP_TOKEN()
        : ui64(0), long_double_(.0) {}

    union {
        struct {
            PP_INTEGRAL_POSTFIX integral_postfix;
            PP_INTEGRAL_TYPE integral_type;
            union {
                int32_t  i32;
                int64_t  i64;
                uint32_t ui32;
                uint64_t ui64;
                //intmax_t integral;
            };
        };
        struct {
            PP_FLOATING_POSTFIX floating_postfix;
            PP_FLOATING_TYPE floating_type;
            union {
                float       float_;
                double      double_;
                long double long_double_;
            };
        };
    };/*
    union {
        intmax_t integral;
        double floating;
    };*/
};

enum entity {
    entity_unknown,

    group,
    group_part,
    if_section,
    if_group,
    elif_groups,
    elif_group,
    else_group,
    endif_line,
    control_line,
    text_line,
    non_directive,
    lparen, // a ( character not immediately preceded by white-space
    identifier_list,
    replacement_list,
    pp_tokens,
    new_line,


    preprocessing_token,

    header_name,
    identifier,
    pp_number,
    integer_literal,
    floating_literal,
    character_literal,
    user_defined_character_literal,
    string_literal,
    user_defined_string_literal,
    preprocessing_op_or_punc,

    h_char_sequence,
    q_char_sequence,
    c_char_sequence,

    identifier_nondigit,
    universal_character_name,

    // simplest
    h_char,
    q_char,
    c_char,
    c_char_basic,
    escape_sequence,
    simple_escape_sequence,
    octal_escape_sequence,
    hexadecimal_escape_sequence,
    unknown_escape_sequence,
    digit,
    octal_digit,
    hexadecimal_digit,
    nondigit,
    sign,
    encoding_prefix,
    s_char_sequence,
    raw_string,
    s_char,
    s_char_basic,

    // other
    e_any,
    whitespace,

    // special parsing tools
    nothing
};