#include "unqualified_id.hpp"
#include "common.hpp"
#include "type_specifier.hpp"

#include <assert.h>


bool eat_operator(parse_state& ps) {
    if (accept(ps, kw_new)) {
        if (accept(ps, "[")) {
            expect(ps, "]");
            // TODO new[]
            return true;
        }
        // TODO:
        return true;
    }
    if (accept(ps, kw_delete)) {
        if (accept(ps, "[")) {
            expect(ps, "]");
            // TODO delete[]
            return true;
        }
        // TODO:
        return true;
    }
    token tok;
    if (!eat_token(ps, tt_operator, &tok)) {
        return false;
    }
    // Operators are sorted by length
    static const char* op_table[] = {
        "->*", "<<=", ">>=", "!=", "%=",
        "&&", "&=", "*=", "++", "+=",
        "--", "-=", "->", "/=", "<<",
        "<=", "==", ">=", ">>", "|=",
        "||", "ˆ=", "%", "&", "(",
        ")", "*", "+", ",", "-",
        "/", "<", "=", ">", "[",
        "]", "|", "~", "ˆ", "!"
    };
    static int op_count = sizeof(op_table) / sizeof(op_table[0]);
    for (int i = 0; i < op_count; ++i) {
        if (tok.str == op_table[i]) {
            // TODO
            return true;
        }
    }
    return false;
}
bool eat_operator_function_id(parse_state& ps) {
    if (!accept(ps, kw_operator)) {
        return false;
    }
    if (!eat_operator(ps)) {
        return false;
    }
    return true;
}/*
bool eat_literal_operator_id(parse_state& ps) {
    if (!accept(ps, kw_operator)) {
        return false;
    }
    if (eat_string_literal(ps)) {
        if (!eat_identifier(ps)) {
            throw parse_exception("Expected an identifier", ps.next_token());
        }
        return true;
    }
    if (eat_user_defined_string_literal(ps)) {
        return true;
    }
    return false;
}*/

bool eat_operator_fn(parse_state& ps) {
    return false;
}

bool eat_unqualified_id_todo(parse_state& ps) {
    // TODO:
    if (accept(ps, "~")) {
        if (eat_class_name(ps, 0)) {
            return true;
        }
        if (eat_decltype_specifier(ps)) {
            return true;
        }
        throw parse_exception("Expected a class name", ps.next_token());
    }

    if (eat_operator_fn(ps)) {
        return true;
    }

    token tok;
    if (eat_token(ps, tt_identifier, &tok)) {
        // TODO: identifier
        // TODO: template_id
    }

    return false;
}
