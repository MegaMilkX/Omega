#include "parse.hpp"


ast_node eat_operator_function_id_2(parse_state& ps) {
    REWIND_SCOPE(operator_function_id_2);
    if (!accept(ps, kw_operator)) {
        return ast_node::null();
    }

    token tok;
    if (!eat_token(ps, tt_operator, &tok)) {
        REWIND_ON_EXIT(operator_function_id_2);
        return ast_node::null();
    }

    return ast_node::make<ast::operator_function_id>(/* TODO */);
}

ast_node eat_literal_operator_id_2(parse_state& ps) {
    // TODO:
    // operator string-literal identifier
    // operator user-defined-string-literal
    return ast_node::null();
}

