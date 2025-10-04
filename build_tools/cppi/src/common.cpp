#include "common.hpp"
#include "parse_exception.hpp"


bool eat_balanced_token(parse_state& ps) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.str == "(") {
        eat_balanced_token_seq(ps);
        tok = ps.next_token();
        if (tok.str != ")") {
            throw parse_exception("Expected a ')'", tok);
        }
    } else if(tok.str == "[") {
        eat_balanced_token_seq(ps);
        tok = ps.next_token();
        if (tok.str != "]") {
            throw parse_exception("Expected a ']'", tok);
        }
    } else if(tok.str == "{") {
        eat_balanced_token_seq(ps);
        tok = ps.next_token();
        if (tok.str != "}") {
            throw parse_exception("Expected a '}'", tok);
        }
    } else if(tok.str != ")" && tok.str != "]" && tok.str != "}" && tok.type != tt_eof) {
        ps.pop_rewind_point();
        return true;
    } else {
        ps.rewind_();
        return false;
    }

    ps.pop_rewind_point();
    return true;
}

bool eat_balanced_token_seq(parse_state& ps) {
    while (eat_balanced_token(ps)) {
        // TODO
    }
    return true;
}

bool eat_balanced_token_except(parse_state& ps, const std::initializer_list<const char*>& exceptions) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.str == "(") {
        eat_balanced_token_seq(ps);
        tok = ps.next_token();
        if (tok.str != ")") {
            throw parse_exception("Expected a ')'", tok);
        }
    } else if(tok.str == "[") {
        eat_balanced_token_seq(ps);
        tok = ps.next_token();
        if (tok.str != "]") {
            throw parse_exception("Expected a ']'", tok);
        }
    } else if(tok.str == "{") {
        eat_balanced_token_seq(ps);
        tok = ps.next_token();
        if (tok.str != "}") {
            throw parse_exception("Expected a '}'", tok);
        }
    } else if(tok.str != ")" && tok.str != "]" && tok.str != "}") {
        for (auto& cstr : exceptions) {
            if (tok.str == cstr) {
                ps.rewind_();
                return false;
            }
        }
        ps.pop_rewind_point();
        return true;
    } else {
        ps.rewind_();
        return false;
    }
}

bool eat_balanced_token_seq_except(parse_state& ps, const std::initializer_list<const char*>& exceptions) {
    while (eat_balanced_token_except(ps, exceptions)) {

    }
    return true;
}
