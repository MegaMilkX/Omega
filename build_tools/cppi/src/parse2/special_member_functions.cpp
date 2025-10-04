#include "parse.hpp"


ast_node eat_conversion_declarator_2(parse_state& ps) {
    ast_node ptrop = eat_ptr_operator_2(ps);
    if (ptrop) {
        return eat_conversion_declarator_2(ps);
    } else {
        return ast_node::make<ast::conversion_declarator>(/* TODO */);
    }
}

ast_node eat_conversion_type_id_2(parse_state& ps) {
    ast_node type_spec_seq = eat_type_specifier_seq_2(ps);
    if (!type_spec_seq) {
        return ast_node::null();
    }

    ast_node conv_decl = eat_conversion_declarator_2(ps);
    return ast_node::make<ast::conversion_type_id>(/* TODO */);
}

ast_node eat_conversion_function_id_2(parse_state& ps) {
    REWIND_SCOPE(conversion_function_id_2);
    if (!accept(ps, kw_operator)) {
        return ast_node::null();
    }

    ast_node n = eat_conversion_type_id_2(ps);
    if (!n) {
        REWIND_ON_EXIT(conversion_function_id_2);
        return ast_node::null();
    }

    return ast_node::make<ast::conversion_function_id>(/* TODO */);
}

