#include "lexer.hpp"


namespace animvm {


    bool lex_identifier(lex_state& ls) {
        ls.begin_token(e_identifier);
        if (!ls.accept_alpha() && !ls.accept_underscore()) {
            return false;
        }
        while (ls.accept_alpha() || ls.accept_alnum() || ls.accept_underscore()) {}
        return ls.end_token();
    }
    bool lex_numeric_literal(lex_state& ls) {
        ls.begin_token(e_literal_integral);
        if (ls.accept('.')) {
            ls.change_token_type(e_literal_floating);
            while (ls.accept_num()) {}
            if (ls.tok_len() == 1) {
                ls.fail_token();
                return false;
            }
        } else {
            while (ls.accept_num()) {}
            if (ls.tok_len() == 0) {
                ls.fail_token();
                return false;
            }
            if (ls.accept('.')) {
                ls.change_token_type(e_literal_floating);
            }
            while (ls.accept_num()) {}
        }
        return ls.end_token();
    }
    bool lex_literal(lex_state& ls) {
        return lex_numeric_literal(ls);
    }
    bool lex_operator(lex_state& ls) {
        ls.begin_token(e_operator);
        const char* operators[] = {
            "@",
            "!=", "!",
            "==", "=",
            ">=", ">",
            "<=", "<",
            "+=", "++", "+",
            "-=", "--", "-",
            "*=", "*",
            "/=", "/",
            "%=", "%",
            "(", ")",
            "[", "]",
            "{", "}",
            ";", ":"
        };
        const size_t count = sizeof(operators) / sizeof(operators[0]);
        for (int i = 0; i < count; ++i) {
            const size_t len = strlen(operators[i]);
            if (strncmp(operators[i], ls.cur, len) == 0) {
                ls.adv(len);
                break;
            }
        }
        return ls.end_token();
    }
    bool lex_whitespace(lex_state& ls) {
        ls.begin_token(e_whitespace);
        while (ls.accept_whitespace()) {}
        return ls.end_token();
    }
    bool lex_eof(lex_state& ls) {
        if (ls.ch() == 0) {
            ls.tok.type = e_eof;
            return true;
        }
        return false;
    }

    token lex_token(lex_state& ls) {
        lex_whitespace(ls)
        || lex_identifier(ls)
        || lex_literal(ls)
        || lex_operator(ls)
        || lex_eof(ls);
        return ls.tok;
    }


}
