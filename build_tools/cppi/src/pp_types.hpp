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

struct PP_FILE;
struct PP_TOKEN {
    PP_FILE* file = 0;
    PP_TOKEN_TYPE type;
    std::string str;
    std::string strlit_content;
    int param_idx = -1;
    int line = -1;
    int col = -1;
    bool is_macro_expandable = true;
    PP_NUMBER_TYPE number_type = PP_NUMBER_NONE;
    union {
        intmax_t integral;
        double floating;
    };
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