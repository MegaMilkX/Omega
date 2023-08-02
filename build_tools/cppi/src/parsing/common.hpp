#pragma once

#include "parse_state.hpp"
#include "parse_exception.hpp"


inline bool eat_token(parse_state& ps, token_type type, const char* string, token* out) {
    ps.push_rewind_point();
    *out = ps.next_token();
    if (out->type != type || out->str != string) {
        ps.rewind_();
        return false;
    }
    ps.pop_rewind_point();
    return true;
}
inline bool eat_token(parse_state& ps, token_type type, token* out) {
    ps.push_rewind_point();
    *out = ps.next_token();
    if (out->type != type) {
        ps.rewind_();
        return false;
    }
    ps.pop_rewind_point();
    return true;
}
inline bool eat_token(parse_state& ps, const char* string, token* out) {
    ps.push_rewind_point();
    *out = ps.next_token();
    if (out->str != string) {
        ps.rewind_();
        return false;
    }
    ps.pop_rewind_point();
    return true;
}
inline bool eat_identifier(parse_state& ps) {
    token tok;
    return eat_token(ps, tt_identifier, &tok);
}
inline bool eat_keyword(parse_state& ps, e_keyword kw) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_keyword || tok.kw_type != kw) {
        ps.rewind_();
        return false;
    }
    ps.pop_rewind_point();
    return true;
}

inline bool accept(parse_state& ps, const char* string) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.str != string) {
        ps.rewind_();
        return false;
    }
    ps.pop_rewind_point();
    return true;
}
inline bool accept(parse_state& ps, e_keyword kw) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_keyword) {
        ps.rewind_();
        return false;
    }
    if (tok.kw_type != kw) {
        ps.rewind_();
        return false;
    }
    ps.pop_rewind_point();
    return true;
}
inline bool expect(parse_state& ps, const char* string) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.str != string) {
        ps.rewind_();
        throw parse_exception((std::string("Expected ") + string).c_str(), tok);
        return false;
    }
    ps.pop_rewind_point();
    return true;
}
inline bool expect(parse_state& ps, e_keyword kw) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_keyword || tok.kw_type != kw) {
        ps.rewind_();
        throw parse_exception("Expected a keyword", tok);
        return false;
    }
    ps.pop_rewind_point();
    return true;
}