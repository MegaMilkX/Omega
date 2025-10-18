#include "parse.hpp"


ast_node eat_dynamic_exception_specification_2(parse_state& ps) {
    if (!accept(ps, kw_throw)) {
        return ast_node::null();
    }

    expect(ps, "(");

    ast_node tid_list = eat_type_id_list_2(ps);

    expect(ps, ")");

    return ast_node::make<ast::dynamic_exception_spec>(std::move(tid_list));
}

ast_node eat_noexcept_specification_2(parse_state& ps) {
    if (!accept(ps, kw_noexcept)) {
        return ast_node::null();
    }

    if (accept(ps, "(")) {
        ast_node const_expr = eat_constant_expression_2(ps);
        expect(ps, ")");

        return ast_node::make<ast::noexcept_spec>(std::move(const_expr));
    }

    return ast_node::make<ast::noexcept_spec>();
}

ast_node eat_exception_specification_2(parse_state& ps) {
    ast_node n = ast_node::null();
    n = eat_dynamic_exception_specification_2(ps);
    if(n) return n;

    n = eat_noexcept_specification_2(ps);
    if(n) return n;

    return ast_node::null();
}

ast_node eat_type_id_list_2(parse_state& ps) {
    ast_node tid = eat_type_id_2(ps);
    if(!tid) return ast_node::null();

    if (accept(ps, "...")) {
        // TODO: Mark as unpack
    }

    ast_node list = ast_node::make<ast::type_id_list>();
    list.as<ast::type_id_list>()->push_back(std::move(tid));

    while (accept(ps, ",")) {
        tid = eat_type_id_2(ps);
        if (!tid) {
            throw parse_exception("Expected type-id", ps.peek_token());
        }

        if (accept(ps, "...")) {
            // TODO: Mark as unpack
        }
        list.as<ast::type_id_list>()->push_back(std::move(tid));
    }

    return list;
}

ast_node eat_exception_declaration_2(parse_state& ps) {
    {
        REWIND_SCOPE(ex_decl);
        bool has_attribs = eat_attribute_specifier_seq(ps);
        ast_node type_spec_seq = eat_type_specifier_seq_2(ps);
        ast_node decl = eat_declarator_2(ps);

        if (!type_spec_seq) {
            REWIND_ON_EXIT(ex_decl);
            goto failure;
        }
        if (!decl) {
            decl = eat_abstract_declarator_2(ps);
        }
        if (has_attribs && !decl) {
            throw parse_exception("No declarator for attribute-specifier-seq", ps.peek_token());
        }

        return ast_node::make<ast::exception_declaration>(std::move(type_spec_seq), std::move(decl));

        failure:;
    }

    if (accept(ps, "...")) {
        return ast_node::make<ast::pack_expansion>();
    }

    return ast_node::null();
}

ast_node eat_handler_2(parse_state& ps) {
    if (!accept(ps, kw_catch)) {
        return ast_node::null();
    }

    expect(ps, "(");
    ast_node ex_decl = eat_exception_declaration_2(ps);
    if(!ex_decl) throw parse_exception("Expected an exception-declaration", ps.peek_token());
    expect(ps, ")");

    ast_node stmt = eat_compound_statement_2(ps);
    if(!stmt) throw parse_exception("Expected a compound-statement", ps.peek_token());
    return ast_node::make<ast::exception_handler>(std::move(ex_decl), std::move(stmt));
}

ast_node eat_handler_seq_2(parse_state& ps) {
    ast_node n = eat_handler_2(ps);
    if(!n) return ast_node::null();

    ast_node seq = ast_node::make<ast::exception_handler_seq>();
    seq.as<ast::exception_handler_seq>()->push_back(std::move(n));

    while (n = eat_handler_2(ps)) {
        seq.as<ast::exception_handler_seq>()->push_back(std::move(n));
    }

    return seq;
}

ast_node eat_function_try_block_2(parse_state& ps) {
    if (!accept(ps, kw_try)) {
        return ast_node::null();
    }

    ast_node ctor_init = eat_ctor_initializer_2(ps);
    ast_node stmt = eat_compound_statement_2(ps);
    if(!stmt) throw parse_exception("Expected a compound-statement", ps.peek_token());

    ast_node handlers = eat_handler_seq_2(ps);
    if(!handlers) throw parse_exception("Expected a handler-seq", ps.peek_token());

    return ast_node::make<ast::function_body>(std::move(ctor_init), std::move(stmt), std::move(handlers));
}

