#include "parse.hpp"
#include "decl_util.hpp"


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

    n = eat_type_id_2(ps);
    if(n) return n;

    n = eat_id_expression_2(ps);
    if(n) return n;

    n = eat_constant_expression_2(ps);
    if (n) return n;

    return ast_node::null();
}

ast_node eat_template_argument_list_2(parse_state& ps) {
    CONTEXT_SCOPE(e_ctx_template_argument_list);

    ast_node arg = eat_template_argument_2(ps);
    if(!arg) return ast_node::null();
    bool is_pack_expansion = false;
    if (accept(ps, "...")) {
        is_pack_expansion = true;
    }

    ast_node list = ast_node::make<ast::template_argument_list>();
    list.as<ast::template_argument_list>()->push_back(std::move(arg), is_pack_expansion);

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
        bool is_pack_expansion = false;
        if (accept(ps, "...")) {
            is_pack_expansion = true;
        }
        list.as<ast::template_argument_list>()->push_back(std::move(arg), true);
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

ast_node eat_simple_template_id_dependent(parse_state& ps) {
    REWIND_SCOPE(simple_template_id_dependent);
    ast_node n = eat_identifier_2(ps);
    if (!n) {
        return ast_node::null();
    }

    if (!accept(ps, "<")) {
        REWIND_ON_EXIT(simple_template_id_dependent);
        return ast_node::null();
    }

    ast_node args = eat_template_argument_list_2(ps);

    expect_closing_angle(ps);

    n = ast_node::make<ast::simple_template_id>(std::move(n), std::move(args));
    return n;
}

ast_node eat_simple_template_id_2(parse_state& ps, symbol_table_ref qualified_scope) {
    REWIND_SCOPE(simple_template_id);
    ast_node n = eat_identifier_2(ps);
    if (!n) {
        return ast_node::null();
    }

    if(ps.is_semantic()) {
        auto ident = n.as<ast::identifier>();
        assert(ident);
        symbol_ref tpl_sym = nullptr;
        if (qualified_scope) {
            tpl_sym = qualified_scope->get_symbol(ident->tok.str, LOOKUP_FLAG_TEMPLATE);
        } else {
            auto cur_scope = ps.get_current_scope();
            tpl_sym = cur_scope->lookup(ident->tok.str, LOOKUP_FLAG_TEMPLATE);
        }
        if (!tpl_sym) {
            REWIND_ON_EXIT(simple_template_id);
            return ast_node::null();
        }
        if (!tpl_sym->is<symbol_template>()) {
            REWIND_ON_EXIT(simple_template_id);
            return ast_node::null();
        }
        ident->sym = tpl_sym;
    }

    if (!accept(ps, "<")) {
        REWIND_ON_EXIT(simple_template_id);
        return ast_node::null();
    }

    ast_node args = eat_template_argument_list_2(ps);

    expect_closing_angle(ps);

    n = ast_node::make<ast::simple_template_id>(std::move(n), std::move(args));
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
        bool is_variadic = false;
        ast_node param;
        if (accept(ps, kw_class)) {
            // ?
        } else if (accept(ps, kw_typename)) {
            // ?
        } else {
            goto failure;
        }

        if (accept(ps, "...")) {
            is_variadic = true;
        }

        ident = eat_identifier_2(ps);
        if (is_variadic) {
            param = ast_node::make<ast::type_parameter>(std::move(ident), is_variadic);
            handle_type_parameter_declaration(ps, param.as<ast::type_parameter>());
            return param;
        }


        if (!accept(ps, "=")) {
            param = ast_node::make<ast::type_parameter>(std::move(ident), is_variadic);
            handle_type_parameter_declaration(ps, param.as<ast::type_parameter>());
            return param;
        }

        tid = eat_type_id_2(ps);
        if(!tid) throw parse_exception("Expected a type-id", ps.peek_token());

        param = ast_node::make<ast::type_parameter>(std::move(ident), std::move(tid), is_variadic);
        handle_type_parameter_declaration(ps, param.as<ast::type_parameter>());
        return param;
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
    return list;
}

ast_node eat_template_class_declaration_2(parse_state& ps) {
    // TODO:
    return ast_node::null();
}

ast_node eat_template_class_definition_2(parse_state& ps) {
    // TODO:
    return ast_node::null();
}

ast_node eat_template_function_declaration_2(parse_state& ps) {
    // TODO:
    return ast_node::null();
}

ast_node eat_template_function_definition_2(parse_state& ps) {
    // TODO:
    return ast_node::null();
}

ast_node eat_template_alias_declaration_2(parse_state& ps) {
    // TODO:
    return ast_node::null();
}

bool finalize_class_template_declaration_2(parse_state& ps, ast_node& decl, symbol_table_ref tpl_scope) {
    auto simple_decl = decl.as<ast::simple_declaration>();
    if(!simple_decl) return false;

    auto decl_spec_seq = simple_decl->decl_spec.as<ast::decl_specifier_seq>();
    if(!decl_spec_seq) return false;

    auto elab_type_spec = decl_spec_seq->get_specifier<ast::elaborated_type_specifier>();
    if(!elab_type_spec) return false;

    if (elab_type_spec->type != e_elaborated_class) {
        return false;
    }

    if (decl_spec_seq->count() > 1) {
        throw parse_exception("template class declaration must be alone in the decl-specifier-sequence", ps.get_latest_token());
    }

    auto ident = elab_type_spec->name.as<ast::identifier>();
    if (!ident) {
        throw parse_exception("template class declaration expected to be an identifier", ps.get_latest_token());
    }
    
    symbol_table_ref cur_scope = ps.get_current_scope();
    int lookup_flags
        = LOOKUP_FLAG_ENUM
        | LOOKUP_FLAG_NAMESPACE
        | LOOKUP_FLAG_TYPEDEF
        | LOOKUP_FLAG_TEMPLATE
        | LOOKUP_FLAG_CLASS;
    symbol_ref tpl_sym = cur_scope->get_symbol(ident->tok.str, lookup_flags);

    if (tpl_sym && !tpl_sym->is<symbol_template>()) {
        throw parse_exception("previously declared symbol is not a template", ps.get_latest_token());
    }

    if(!tpl_sym) {
        tpl_sym = ps.create_symbol<symbol_template>(nullptr, ident->tok.str.c_str());
        tpl_sym->nested_symbol_table = tpl_scope;
        tpl_scope->set_owner(tpl_sym);
    } else {
        // TODO: Compare template parameters, append default parameters?
        dbg_printf_color("template '%s' redeclared\n", DBG_YELLOW, ident->tok.str.c_str());
    }

    return true;
}

bool finalize_class_template_definition_2(parse_state& ps, ast_node& decl, symbol_table_ref tpl_scope) {
    auto simple_decl = decl.as<ast::simple_declaration>();
    if(!simple_decl) return false;

    auto decl_spec_seq = simple_decl->decl_spec.as<ast::decl_specifier_seq>();
    if(!decl_spec_seq) return false;

    auto class_spec = decl_spec_seq->get_specifier<ast::class_specifier>();
    if(!class_spec) return false;

    if (decl_spec_seq->count() > 1) {
        throw parse_exception("template class-specifier must be alone in the decl-specifier-sequence", ps.get_latest_token());
    }

    if (simple_decl->declarator_list) {
        throw parse_exception("template class declaration must not contain any declarators", ps.get_latest_token());
    }

    auto class_head = class_spec->head.as<ast::class_head>();
    if (!class_head) {
        throw parse_exception("template class must have class-head", ps.get_latest_token());
    }

    if (!class_head->class_head_name) {
        throw parse_exception("template class must have a name", ps.get_latest_token());
    }

    auto ident = class_head->class_head_name.as<ast::identifier>();
    if (!ident) {
        throw parse_exception("TODO: only identifier as a template class name supported", ps.get_latest_token());
    }

    symbol_table_ref cur_scope = ps.get_current_scope();
    int lookup_flags
        = LOOKUP_FLAG_ENUM
        | LOOKUP_FLAG_NAMESPACE
        | LOOKUP_FLAG_TYPEDEF
        | LOOKUP_FLAG_TEMPLATE
        | LOOKUP_FLAG_CLASS;
    symbol_ref tpl_sym = cur_scope->get_symbol(ident->tok.str, lookup_flags);

    if (tpl_sym && !tpl_sym->is<symbol_template>()) {
        throw parse_exception("previously declared symbol is not a template", ps.get_latest_token());
    }

    if(!tpl_sym) {
        tpl_sym = ps.create_symbol<symbol_template>(nullptr, ident->tok.str.c_str());
        tpl_sym->nested_symbol_table = tpl_scope;
        tpl_scope->set_owner(tpl_sym);
    } else {
        // TODO: Compare template parameters
        // TODO: Append declaration with a definition
        dbg_printf_color("previously declared template '%s' defined\n", DBG_YELLOW, ident->tok.str.c_str());
    }

    return true;
}

ast_node eat_template_declaration_2(parse_state& ps) {
    REWIND_SCOPE(template_decl);
    if (!accept(ps, kw_template)) {
        return ast_node::null();
    }

    expect(ps, "<");

    symbol_table_ref tpl_scope = ps.enter_scope(scope_template);

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

    ps.exit_scope();
    /*
    finalize_class_template_definition_2(ps, decl, tpl_scope)
    || finalize_class_template_declaration_2(ps, decl, tpl_scope);
    */
    return ast_node::make<ast::template_declaration>(std::move(param_list), std::move(decl));
}


