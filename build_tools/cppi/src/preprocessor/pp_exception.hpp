#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <exception>
#include "char_provider.hpp"
#include "pp_state.hpp"


inline void emit_warning(const char* fmt, const pp_state* pps, ...) {
    va_list args;
    va_start(args, pps);
    char buf[256];
    vsnprintf(buf, 256, fmt, args);
    va_end(args);

    printf("pp warning: '%s' line %i, col %i: %s\n", pps->get_file_name().c_str(), pps->get_line(), pps->get_col(), buf);
}

class pp_exception : public std::exception {
    std::string msg;
    int line;
    int col;
public:
    pp_exception(const char* fmt, PP_TOKEN tok, ...)
    : line(tok.line), col(tok.col) {
        va_list args;
        va_start(args, tok);
        char buf[256];
        vsnprintf(buf, 256, fmt, args);
        va_end(args);

        char buf2[256];
        std::string fname = tok.file ? tok.file->file_name : "";
        snprintf(buf2, 256, "pp error: '%s' line %i, col %i: %s", fname.c_str(), line, col, buf);

        this->msg = buf2;
    }
    pp_exception(const char* fmt, int line, int col, ...)
    : line(line), col(col) {
        va_list args;
        va_start(args, col);
        char buf[256];
        vsnprintf(buf, 256, fmt, args);
        va_end(args);

        char buf2[256];
        snprintf(buf2, 256, "pp error: line %i, col %i: %s", line, col, buf);

        this->msg = buf2;
    }
    pp_exception(const char* msg, const pp_state& pps)
        : pp_exception(msg, pps.get_line(), pps.get_col()) {}
    pp_exception(const char* msg, const char_provider& rs)
        : pp_exception(msg, rs.get_line(), rs.get_col()) {}
    char const* what() const override {
        return msg.c_str();
    }
};
class pp_exception_notimplemented : public std::exception {
    std::string msg;
    int line;
    int col;
public:
    pp_exception_notimplemented(const char* msg, int line, int col)
    : line(line), col(col) {
        char buf[256];
        snprintf(buf, 256, "!PP NOT IMPLEMENTED!: line %i, col %i: %s", line, col, msg);
        this->msg = buf;
    }
    pp_exception_notimplemented(const char* msg, const pp_state& pps)
        : pp_exception_notimplemented(msg, pps.get_line(), pps.get_col()) {}
    pp_exception_notimplemented(const char* msg, const PP_TOKEN& tok)
        : pp_exception_notimplemented(msg, tok.line, tok.col) {}
    pp_exception_notimplemented(const char* msg, const char_provider& rs)
        : pp_exception_notimplemented(msg, rs.get_line(), rs.get_col()) {}
    char const* what() const override {
        return msg.c_str();
    }
};