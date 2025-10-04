#include "parse.hpp"


ast_node eat_identifier_2(parse_state& ps) {
    token tok;
    if (!eat_token(ps, tt_identifier, &tok)) {
        return ast_node::null();
    }
    return ast_node::make<ast::identifier>(tok);
}