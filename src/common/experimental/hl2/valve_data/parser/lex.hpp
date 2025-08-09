#pragma once

#include <assert.h>
#include "log/log.hpp"
#include "token.hpp"


namespace valve {

struct lex_state {
    uint64_t size;
    const char* data;
    uint64_t at;
    uint32_t line;
    uint32_t col;
    token tok;
    bool m_has_error = false;

    lex_state(const char* data, uint64_t size)
        : size(size),
        data(data),
        at(0),
        line(0),
        col(0),
        tok{ 
            .begin = 0,
            .end = 0,
            .line = 0,
            .col = 0,
            .type = tok_none
        }
    {}

    char peek() const {
        if (is_eof()) {
            return '\0';
        }
        return data[at];
    }
    void advance(int count) {
        for(int i = 0; i < count; ++i) {
            if (is_eof()) {
                break;
            }
            if (peek() == '\n') {
                ++line;
                col = 0;
            } else {
                ++col;
            }
            ++at;
        }
    }

    void set_error(const char* desc) {
        LOG_ERR("lexer error: " << desc);
        m_has_error = true;
    }
    bool has_error() const {
        return m_has_error;
    }

    bool is_char(char c) const {
        return peek() == c;
    }
    bool is_space() const {
        return isspace(peek());
    }
    bool is_eof() const {
        return at >= size;
    }

    void begin_token(tok_type type) {
        tok.line = line;
        tok.col = col;
        tok.type = type;
        tok.begin = at;
    }
    bool end_token() {
        tok.end = at;
        if (tok.begin == tok.end) {
            tok.type = tok_none;
            return false;
        }
        return true;
    }

    void rewind() {
        at = tok.begin;
        line = tok.line;
        col = tok.col;
    }
};

bool lex_accept(lex_state& ls, char c);
bool lex_whitespace(lex_state& ls);
bool lex_comment(lex_state& ls);
bool lex_string(lex_state& ls);
bool lex_punctuator(lex_state& ls);
bool lex_eof(lex_state& ls);
bool lex_token(lex_state& ls);

}

