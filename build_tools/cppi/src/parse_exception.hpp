#pragma once

#include <assert.h>
#include <exception>
#include "token.hpp"


class parse_exception : public std::exception {
    std::string msg;
    std::string tok_str;
    int line;
    int col;
public:
    parse_exception(const char* msg, const char* fname, const char* tok_str, int line, int col)
    : line(line), col(col) 
    {
        char buf[256];
        snprintf(buf, 256, "parse error: '%s': line %i, col %i '%s': %s", fname, line, col, tok_str, msg);
        this->msg = buf;
        assert(false);
    }
    parse_exception(const char* msg, const token& tok)
        : parse_exception(msg, tok.file->file_name.c_str(), tok.str.c_str(), tok.line, tok.col)
    {}
    char const* what() const override {
        return msg.c_str();
    }
};
