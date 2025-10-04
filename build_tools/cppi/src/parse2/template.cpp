#include "parse.hpp"


ast_node eat_typename_specifier_2(parse_state& ps) {
    if (!accept(ps, kw_typename)) {
        return ast_node::null();
    }

    ast_node nns = eat_nested_name_specifier_2(ps);
    if (!nns) {
        throw parse_exception("Expected a nested-name-specifier", ps.peek_token());
    }

    if (accept(ps, kw_template)) {
        ast_node stid = eat_simple_template_id_2(ps);
        if (!stid) {
            throw parse_exception("Expected a simple-template-id", ps.peek_token());
        }
        nns.as<ast::nested_name_specifier>()->push_next(std::move(stid));
        return nns;
    } else {
        ast_node stid = eat_simple_template_id_2(ps);
        if (stid) {
            nns.as<ast::nested_name_specifier>()->push_next(std::move(stid));
            return nns;
        }
        ast_node ident = eat_identifier_2(ps);
        if (ident) {
            nns.as<ast::nested_name_specifier>()->push_next(std::move(stid));
            return nns;
        }
        throw parse_exception("Expected an identifier", ps.peek_token());
    }
    return ast_node::null();
}

ast_node eat_template_argument_2(parse_state& ps) {
    ast_node n = ast_node::null();

    n = eat_id_expression_2(ps);
    if(n) return n;

    n = eat_type_id_2(ps);
    if(n) return n;

    n = eat_constant_expression_2(ps);
    if (n) return n;

    return ast_node::null();
}

ast_node eat_template_argument_list_2(parse_state& ps) {
    CONTEXT_SCOPE(e_ctx_template_argument_list);

    ast_node arg = eat_template_argument_2(ps);
    if(!arg) return ast_node::null();

    ast_node list = ast_node::make<ast::template_argument_list>();
    list.as<ast::template_argument_list>()->push_back(std::move(arg));

    while (true) {
        REWIND_SCOPE(tpl_arg_list_2_loop);
        if (!accept(ps, ",")) {
            break;
        }
        arg = eat_template_argument_2(ps);
        if (!arg) {
            REWIND_ON_EXIT(tpl_arg_list_2_loop);
            break;
        }
        list.as<ast::template_argument_list>()->push_back(std::move(arg));
    }
    return list;
}

void expect_closing_angle(parse_state& ps) {
    token tok;
    if (!eat_token(ps, ">", &tok)) {
        if (!eat_token(ps, ">>", &tok)) {
            throw parse_exception("Expected a '>'", tok);
        }
        ps.get_latest_token().str = ">";

        token tk = tok;
        tk.str = ">";
        ps.push_token(tk);
    }
}

ast_node eat_simple_template_id_2(parse_state& ps) {
    ps.push_rewind_point();
    ast_node n = eat_identifier_2(ps);
    if (!n) {
        ps.pop_rewind_point();
        return ast_node::null();
    }

    token tok;
    if (!accept(ps, "<")) {
        ps.rewind_();
        return ast_node::null();
    }

    ast_node nargs = eat_template_argument_list_2(ps);

    expect_closing_angle(ps);

    n = ast_node::make<ast::simple_template_id>(std::move(n), std::move(nargs));

    ps.pop_rewind_point();
    return n;
}

ast_node eat_template_id_2(parse_state& ps) {
    ast_node n = eat_simple_template_id_2(ps);
    if(n) return n;

    ps.push_rewind_point();
    n = eat_operator_function_id_2(ps);
    if (n) {
        token tok;
        if (!accept(ps, "<")) {
            ps.rewind_();
            return ast_node::null();
        }
        ast_node nargs = eat_template_argument_list_2(ps);
        expect_closing_angle(ps);
        
        ps.pop_rewind_point();
        return ast_node::make<ast::template_id>(std::move(n), std::move(nargs));
    }

    n = eat_literal_operator_id_2(ps);
    if (n) {
        token tok;
        if (!accept(ps, "<")) {
            ps.rewind_();
            return ast_node::null();
        }
        ast_node nargs = eat_template_argument_list_2(ps);
        expect_closing_angle(ps);

        ps.pop_rewind_point();
        return ast_node::make<ast::template_id>(std::move(n), std::move(nargs));
    }
    ps.pop_rewind_point();
    return ast_node::null();
}

ast_node eat_type_parameter_2(parse_state& ps) {
    {
        REWIND_SCOPE(class_or_typename);
        ast_node ident = ast_node::null();
        ast_node tid = ast_node::null();
        if (accept(ps, kw_class)) {
            // ?
        } else if (accept(ps, kw_typename)) {
            // ?
        } else {
            goto failure;
        }

        bool is_variadic = false;
        if (accept(ps, "...")) {
            is_variadic = true;
        }

        ident = eat_identifier_2(ps);
        if (is_variadic) {
            return ast_node::make<ast::type_parameter>(std::move(ident), is_variadic);
        }

        if (!accept(ps, "=")) {
            return ast_node::make<ast::type_parameter>(std::move(ident), is_variadic);
        }

        tid = eat_type_id_2(ps);
        if(!tid) throw parse_exception("Expected a type-id", ps.peek_token());

        return ast_node::make<ast::type_parameter>(std::move(ident), std::move(tid), is_variadic);
    failure:
        REWIND_ON_EXIT(class_or_typename);
    }

    // TODO:
    // template < template-parameter-list > class ...opt identifieropt
    // template < template-parameter-list > class identifieropt = id-expression

    return ast_node::null();
}

ast_node eat_template_parameter_2(parse_state& ps) {
    ast_node n = ast_node::null();
    n = eat_type_parameter_2(ps);
    if(n) return n;
    n = eat_parameter_declaration_2(ps);
    if(n) return n;
    return ast_node::null();
}

ast_node eat_template_parameter_list_2(parse_state& ps) {
    ast_node n = eat_template_parameter_2(ps);
    if(!n) return ast_node::null();

    ast_node list = ast_node::make<ast::template_parameter_list>();
    list.as<ast::template_parameter_list>()->push_back(std::move(n));

    while (accept(ps, ",")) {
        n = eat_template_parameter_2(ps);
        list.as<ast::template_parameter_list>()->push_back(std::move(n));
    }
    return std::move(list);
}

ast_node eat_template_declaration_2(parse_state& ps) {
    REWIND_SCOPE(template_decl);
    if (!accept(ps, kw_template)) {
        return ast_node::null();
    }

    expect(ps, "<");

    ast_node param_list = eat_template_parameter_list_2(ps);
    if (!param_list) {
        REWIND_ON_EXIT(template_decl);
        return ast_node::null();
    }
    expect_closing_angle(ps);

    ast_node decl = eat_declaration_2(ps);
    if (!decl) {
        throw parse_exception("Expected a declaration", ps.peek_token());
    }

    return ast_node::make<ast::template_declaration>(std::move(param_list), std::move(decl));
}

