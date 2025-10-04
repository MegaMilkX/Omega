#include "parse.hpp"


ast_node eat_attribute_specifier_seq_2(parse_state& ps) {
    attribute_specifier sp;
    if (!eat_attribute_specifier_seq(ps, sp)) {
        return ast_node::null();
    }
    return ast_node::make<ast::attributes>(std::move(sp));
}

