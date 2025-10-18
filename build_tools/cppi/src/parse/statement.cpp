#include "parse.hpp"


ast_node eat_compound_statement_2(parse_state& ps) {
    if (!accept(ps, "{")) {
        return ast_node::null();
    }

    eat_balanced_token_seq(ps);

    expect(ps, "}");
    return ast_node::make<ast::compound_statement>();
}

