#include "lex.hpp"


namespace valve {

bool lex_accept(lex_state& ls, char c) {
    if (ls.is_char(c)) {
        ls.advance(1);
        return true;
    }
    return false;
}

bool lex_whitespace(lex_state& ls) {
    ls.begin_token(tok_whitespace);
    while (ls.is_space() && ls.peek() != '\0') {
        ls.advance(1);
    }
    return ls.end_token();
}
bool lex_comment(lex_state& ls) {
    ls.begin_token(tok_comment);
    if (!lex_accept(ls, '/')) {
        return ls.end_token();
    }
    if (lex_accept(ls, '/')) {
        while (ls.peek() != '\n' && !ls.is_eof()) {
            ls.advance(1);
        }
    } else {
        ls.rewind();
    }
    return ls.end_token();
}
bool lex_string(lex_state& ls) {
    ls.begin_token(tok_string);
    if (lex_accept(ls, '"')) {
        while (1) {
            if (ls.peek() == '\n') {
                ls.set_error("Unexpected newline character");
                return false;
            } else if (ls.is_eof()) {
                ls.set_error("Unexpected end of file");
                return false;
            }
            if (ls.peek() == '"') {
                ls.advance(1);
                break;
            }
            ls.advance(1);
        }
    } else {
        while (1) {
            if (isspace(ls.peek()) || ls.is_eof()) {
                break;
            }
            ls.advance(1);
        }
    }
    return ls.end_token();
}
bool lex_punctuator(lex_state& ls) {
    ls.begin_token(tok_punct);
    switch (ls.peek()) {
    case '{':
    case '}':
        ls.advance(1);
    }
    return ls.end_token();
}
bool lex_eof(lex_state& ls) {
    if (ls.is_eof()) {
        ls.tok.type = tok_eof;
        ls.tok.begin = ls.at;
        ls.tok.end = ls.at;
        return true;
    } else {
        return false;
    }
}
bool lex_token(lex_state& ls) {
    return lex_whitespace(ls)
        || lex_comment(ls)
        || lex_punctuator(ls)
        || lex_string(ls)
        || lex_eof(ls);
}

}

