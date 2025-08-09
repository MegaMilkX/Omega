#pragma once

#include <stdint.h>


namespace valve {

enum tok_type {
    tok_none,
    tok_error,
    tok_eof,
    tok_string,
    tok_punct,
    tok_whitespace,
    tok_comment
};

struct token {
    uint32_t begin;
    uint32_t end;
    uint32_t line;
    uint32_t col;
    tok_type type;
};

}

