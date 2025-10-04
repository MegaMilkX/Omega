#pragma once

#include "parse_state.hpp"
#include "parse_exception.hpp"
#include "ast/ast.hpp"


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
inline bool eat_token(parse_state& ps, e_keyword kw, token* out) {
    ps.push_rewind_point();
    *out = ps.next_token();
    if (out->type != tt_keyword) {
        ps.rewind_();
        return false;
    }
    if (out->kw_type != kw) {
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

inline bool peek(parse_state& ps, const char* string) {
    token tok = ps.peek_token();
    if (tok.str != string) {
        return false;
    }
    return true;
}
inline bool peek(parse_state& ps, e_keyword kw) {
    token tok = ps.peek_token();
    if (tok.kw_type != kw) {
        return false;
    }
    return true;
}

inline bool accept(parse_state& ps, const char* string) {
    REWIND_SCOPE(accept_str);
    token tok = ps.next_token();
    if (tok.str != string) {
        REWIND_ON_EXIT(accept_str);
        return false;
    }
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

bool eat_balanced_token(parse_state& ps);
bool eat_balanced_token_seq(parse_state& ps);
bool eat_balanced_token_except(parse_state& ps, const std::initializer_list<const char*>& exceptions);
bool eat_balanced_token_seq_except(parse_state& ps, const std::initializer_list<const char*>& exceptions);

