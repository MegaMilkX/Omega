#include "parse.hpp"
#include "decl_util.hpp"


ast_node eat_initializer_clause_2(parse_state& ps) {
    ast_node n = eat_braced_init_list_2(ps);
    if (n) {
        return n;
    }
    n = eat_assignment_expression_2(ps);
    return n;
}

ast_node eat_initializer_list_2(parse_state& ps) {
    ast_node init_clause = eat_initializer_clause_2(ps);
    if (!init_clause) {
        return ast_node::null();
    }

    ast_node init_list = ast_node::make<ast::init_list>();
    init_list.as<ast::init_list>()->add(std::move(init_clause));
    if (accept(ps, "...")) {
        // TODO: mark unpack
    }
    if (!accept(ps, ",")) {
        return init_list;
    }

    while (init_clause = eat_initializer_clause_2(ps)) {
        init_list.as<ast::init_list>()->add(std::move(init_clause));
        if (accept(ps, "...")) {
            // TODO: mark unpack
        }
        if (!accept(ps, ",")) {
            break;
        }
    }
    return init_list;
}

ast_node eat_braced_init_list_2(parse_state& ps) {
    if (!accept(ps, "{")) {
        return ast_node::null();
    }

    ast_node init_list = eat_initializer_list_2(ps);
    // Optional , already handled by eat_initializer_list_2()
    /*if (init_list) {
        accept(ps, ",");
    }*/

    expect(ps, "}");

    return ast_node::make<ast::braced_init_list>(std::move(init_list));
}

ast_node eat_cv_qualifier_2(parse_state& ps, decl_flags& flags) {
    if (accept(ps, kw_const)) {
        flags.add(e_cv_const);
        return ast_node::make<ast::cv_qualifier>(cv_const);
    }

    if (accept(ps, kw_volatile)) {
        flags.add(e_cv_volatile);
        return ast_node::make<ast::cv_qualifier>(cv_volatile);
    }

    return ast_node::null();
}

ast_node eat_cv_qualifier_seq_2(parse_state& ps, decl_flags& flags) {
    ast_node cv = eat_cv_qualifier_2(ps, flags);
    if(!cv) return ast_node::null();

    ast_node cv_seq = ast_node::make<ast::cv_qualifier_seq>();
    cv_seq.as<ast::cv_qualifier_seq>()->push_back(std::move(cv));

    while (cv = eat_cv_qualifier_2(ps, flags)) {
        cv_seq.as<ast::cv_qualifier_seq>()->push_back(std::move(cv));
    }
    return cv_seq;
}

ast_node eat_parameter_declaration_2(parse_state& ps) {
    ast_node decl_spec = ast_node::null();
    {
        REWIND_SCOPE(parameter_declaration_2_decl_spec);
        eat_attribute_specifier_seq(ps);

        decl_spec = eat_decl_specifier_seq_2(ps);
        if (!decl_spec) {
            REWIND_ON_EXIT(parameter_declaration_2_decl_spec);
            return ast_node::null();
        }
    }

    ast_node declarator = eat_declarator_2(ps);
    if (!declarator) {
        declarator = eat_abstract_declarator_2(ps);
    }


    if (!accept(ps, "=")) {
        ast_node param_decl = ast_node::make<ast::parameter_declaration>(std::move(decl_spec), std::move(declarator));
        handle_parameter_declaration(ps, param_decl.as<ast::parameter_declaration>());
        return param_decl;
    }

    ast_node initializer = eat_initializer_clause_2(ps);
    if (!initializer) {
        throw parse_exception("Expected an initializer", ps.peek_token());
    }

    ast_node param_decl = ast_node::make<ast::parameter_declaration>(std::move(decl_spec), std::move(declarator), std::move(initializer));
    handle_parameter_declaration(ps, param_decl.as<ast::parameter_declaration>());
    return param_decl;
}

ast_node eat_parameter_declaration_list_2(parse_state& ps) {
    ast_node n = eat_parameter_declaration_2(ps);
    if(!n) return ast_node::null();

    ast_node list = ast_node::make<ast::parameter_declaration_list>();
    list.as<ast::parameter_declaration_list>()->push_back(std::move(n));

    while (true) {
        REWIND_SCOPE(param_decl_list_2_loop);
        if (!accept(ps, ",")) {
            break;
        }
        n = eat_parameter_declaration_2(ps);
        if (!n) {
            REWIND_ON_EXIT(param_decl_list_2_loop);
            break;
        }

        list.as<ast::parameter_declaration_list>()->push_back(std::move(n));
    }

    return list;
}

ast_node eat_parameter_declaration_clause_2(parse_state& ps) {
    ast_node n = eat_parameter_declaration_list_2(ps);
    if (n) {
        if (accept(ps, ",")) {
            expect(ps, "...");
            n.as<ast::parameter_declaration_list>()->is_variadic = true;
        }
    }
    return n;
}

ast_node eat_ref_qualifier_2(parse_state& ps) {
    if (accept(ps, "&")) {
        return ast_node::make<ast::ref_qualifier>(e_ref);
    }
    if (accept(ps, "&&")) {
        return ast_node::make<ast::ref_qualifier>(e_rvref);
    }
    return ast_node::null();
}

ast_node eat_parameters_and_qualifiers_2(parse_state& ps) {
    if (!accept(ps, "(")) {
        return ast_node::null();
    }

    ps.enter_scope(scope_function_parameters);

    ast_node n = eat_parameter_declaration_clause_2(ps);
    expect(ps, ")");
    ps.exit_scope();

    // These are all optional
    decl_flags decl_flags_;
    ast_node cv = eat_cv_qualifier_seq_2(ps, decl_flags_);
    ast_node ref = eat_ref_qualifier_2(ps);
    ast_node ex_spec = eat_exception_specification_2(ps);
    eat_attribute_specifier_seq(ps);

    return ast_node::make<ast::parameters_and_qualifiers>(
        std::move(n), std::move(cv), std::move(ref), std::move(ex_spec),
        decl_flags_.has(e_cv_const), decl_flags_.has(e_cv_volatile)
    );
}

ast_node eat_ptr_declarator_core_2(parse_state& ps, ast_node& first, ast_node*& last, bool is_abstract) {
    ast_node ptr_op = ast_node::null();
    ptr_op = eat_ptr_operator_2(ps);
    
    if (ptr_op) {
        ptr_op = ast_node::make<ast::ptr_declarator>(std::move(ptr_op), ast_node::null());
        ast_node n = eat_ptr_declarator_core_2(ps, first, last, is_abstract);
        if (is_abstract && first.is_null()) {
            first = ast_node::make<ast::declarator>();
            last = &first;
        }
        if (!is_abstract && !last) {
            return ast_node::null();
        }

        last->as<ast::declarator_part>()->next = std::move(ptr_op);
        last = &last->as<ast::declarator_part>()->next;

        return ast_node::make<ast::dummy>();
    }

    return eat_noptr_declarator_core_2(ps, first, last, is_abstract);
}

ast_node eat_noptr_declarator_secondary_core_2(parse_state& ps, ast_node& first, ast_node*& last, bool is_abstract) {
    if (accept(ps, "[")) {
        ast_node const_expr = eat_constant_expression_2(ps);
        expect(ps, "]");
        eat_attribute_specifier_seq(ps);

        ast_node noptr = ast_node::make<ast::array_declarator>(std::move(const_expr));
        if (is_abstract && first.is_null()) {
            first = ast_node::make<ast::declarator>();
            last = &first;
        }
        last->as<ast::declarator_part>()->next = std::move(noptr);
        last = &last->as<ast::declarator_part>()->next;

        ast_node n = eat_noptr_declarator_secondary_core_2(ps, first, last, is_abstract);

        return ast_node::make<ast::dummy>();
    }

    {
        // Need to back off if there's a -> after
        REWIND_SCOPE(noptr_declarator_secondary_core_2_parameters_and_qualifiers);
        ast_node paq = eat_parameters_and_qualifiers_2(ps);
        if (paq) {
            if (accept(ps, "->")) {
                REWIND_ON_EXIT(noptr_declarator_secondary_core_2_parameters_and_qualifiers);
                return ast_node::null();
            }

            if (is_abstract && first.is_null()) {
                first = ast_node::make<ast::declarator>();
                last = &first;
            }
            last->as<ast::declarator_part>()->next = std::move(paq);
            last = &last->as<ast::declarator_part>()->next;


            ast_node n = eat_noptr_declarator_secondary_core_2(ps, first, last, is_abstract);

            return ast_node::make<ast::ptr_declarator>();
        }
    }
    return ast_node::null();
}

ast_node eat_noptr_declarator_paren_core_2(parse_state& ps, ast_node& first, ast_node*& last, bool is_abstract) {
    REWIND_SCOPE(noptr_declarator_paren_core_2);
    if (!accept(ps, "(")) {
        return ast_node::null();
    }

    ast_node n = eat_ptr_declarator_core_2(ps, first, last, is_abstract);
    if (!n) {
        REWIND_ON_EXIT(noptr_declarator_paren_core_2);
        return ast_node::null();
    }
    expect(ps, ")");

    return ast_node::make<ast::dummy>();
}

ast_node eat_noptr_declarator_core_2(parse_state& ps, ast_node& first, ast_node*& last, bool is_abstract) {
    if (eat_noptr_declarator_paren_core_2(ps, first, last, is_abstract)) {
        // Nothing
    } else if (!is_abstract) {
        ast_node decl = eat_declarator_id_2(ps);
        if (!decl) {
            // TODO: Maybe can make it abstract instead of returning completely?
            return ast_node::null();
        }
        eat_attribute_specifier_seq(ps);

        assert(first.is_null());
        first = ast_node::make<ast::declarator>(std::move(decl));
        last = &first;
    }

    return eat_noptr_declarator_secondary_core_2(ps, first, last, is_abstract);
}

ast_node eat_declarator_core_2(parse_state& ps, bool is_abstract) {
    REWIND_SCOPE(declarator_core);
    ast_node first;
    ast_node* last = nullptr;
    eat_ptr_declarator_core_2(ps, first, last, is_abstract);
    
    // TODO: noptr_abstract_declarator(opt) parameters-and-qualifiers trailing-return-type

    if (!last) {
        REWIND_ON_EXIT(declarator_core);
        return ast_node::null();
    }

    return first;
}

ast_node eat_declarator_2(parse_state& ps) {
    return eat_declarator_core_2(ps, false);
}

ast_node eat_abstract_declarator_2(parse_state& ps) {
    return eat_declarator_core_2(ps, true);
}

ast_node eat_ptr_operator_2(parse_state& ps) {
    if (accept(ps, "*")) {
        eat_attribute_specifier_seq(ps);
        decl_flags decl_flags_;
        eat_cv_qualifier_seq_2(ps, decl_flags_);
        return ast_node::make<ast::ptr_operator>(e_ptr, decl_flags_.has(e_cv_const), decl_flags_.has(e_cv_volatile));
    }

    if (accept(ps, "&")) {
        eat_attribute_specifier_seq(ps);
        return ast_node::make<ast::ptr_operator>(e_ref);
    }

    if (accept(ps, "&&")) {
        eat_attribute_specifier_seq(ps);
        return ast_node::make<ast::ptr_operator>(e_rvref);
    }

    // TODO: This part is unfinished, seems to parse ok, but does not result in a proper ast
    if(eat_nested_name_specifier_2(ps)) {
        if (!accept(ps, "*")) {
            throw parse_exception("Expected a *", ps.peek_token());
        }

        eat_attribute_specifier_seq(ps);
        decl_flags decl_flags_;
        eat_cv_qualifier_seq_2(ps, decl_flags_);

        // TODO: nested name

        return ast_node::make<ast::ptr_operator>(e_ptr);
    }
    return ast_node::null();
}

ast_node eat_declarator_id_2(parse_state& ps) {
    REWIND_SCOPE(declarator_id_2);
    if (accept(ps, "...")) {
        // TODO: something about variadics
    }

    ps.push_context(e_ctx_declarator_name);
    ast_node n = eat_id_expression_2(ps);
    ps.pop_context();
    if (!n) {
        REWIND_ON_EXIT(declarator_id_2);
        return ast_node::null();
    }

    return n;
}

ast_node eat_type_id_2(parse_state& ps) {
    ast_node type_spec_seq = eat_type_specifier_seq_2(ps);
    if (!type_spec_seq) {
        return ast_node::null();
    }

    ast_node declarator = eat_abstract_declarator_2(ps);

    return ast_node::make<ast::type_id>(std::move(type_spec_seq), std::move(declarator));
}

ast_node eat_brace_or_equal_initializer_2(parse_state& ps) {
    if (accept(ps, "=")) {
        ast_node init_clause = eat_initializer_clause_2(ps);
        if(!init_clause) throw parse_exception("Expected an initializer-clause", ps.peek_token());
        return ast_node::make<ast::equal_initializer>(std::move(init_clause));
    }

    ast_node braced_init_list = eat_braced_init_list_2(ps);
    if(braced_init_list) return braced_init_list;

    return ast_node::null();
}

ast_node eat_function_body_2(parse_state& ps) {
    {
        REWIND_SCOPE(function_body);
        ast_node ctor_init = eat_ctor_initializer_2(ps);
        ast_node stmt = eat_compound_statement_2(ps);
        if (stmt) {
            return ast_node::make<ast::function_body>(std::move(ctor_init), std::move(stmt));
        }
        REWIND_ON_EXIT(function_body);
    }

    ast_node n = eat_function_try_block_2(ps);
    if(n) return n;

    {
        REWIND_SCOPE(default_delete);
        if (accept(ps, "=")) {
            if (accept(ps, kw_default)) {
                expect(ps, ";");
                return ast_node::make<ast::function_body_default>();
            } else if (accept(ps, kw_delete)) {
                expect(ps, ";");
                return ast_node::make<ast::function_body_delete>();
            }
            REWIND_ON_EXIT(default_delete);
        }
    }
    return ast_node::null();
}

ast_node eat_initializer_2(parse_state& ps) {
    REWIND_SCOPE(initializer);
    ast_node brace_or_eq = eat_brace_or_equal_initializer_2(ps);
    if (brace_or_eq) {
        return brace_or_eq;
    }

    if (accept(ps, "(")) {
        ast_node expr_list = eat_expression_list_2(ps);
        if (!expr_list) {
            REWIND_ON_EXIT(initializer);
            return ast_node::null();
        }
        expect(ps, ")");
        return expr_list;
    }

    return ast_node::null();
}

ast_node eat_init_declarator_2(parse_state& ps, const ast::decl_specifier_seq* dsseq) {
    ast_node d = eat_declarator_2(ps);
    if(!d) return ast_node::null();

    // Declare symbol
    {
        if (!dsseq) {
            throw parse_exception("declaration without any specifiers is not allowed", ps.get_latest_token());
        }
        const ast::declarator* pdeclarator = d.as<ast::declarator>();
        if (dsseq->flags.has(e_typedef)) {
            declare_typedef(ps, dsseq, pdeclarator);
        } else {
            if (pdeclarator->is_function()) {
                declare_function(ps, dsseq, pdeclarator);
            } else { // is an object declaration and definition
                declare_object(ps, dsseq, pdeclarator);
            }
        }
    }

    if (d.as<ast::declarator>()->is_function()) {
        return ast_node::make<ast::init_declarator>(std::move(d), ast_node::null());
    }
    ast_node init = eat_initializer_2(ps);
    return ast_node::make<ast::init_declarator>(std::move(d), std::move(init));
}

ast_node eat_init_declarator_list_2(parse_state& ps, const ast::decl_specifier_seq* dsseq) {
    ast_node n = eat_init_declarator_2(ps, dsseq);
    if(!n) return ast_node::null();

    ast_node list = ast_node::make<ast::init_declarator_list>();
    list.as<ast::init_declarator_list>()->push_back(std::move(n));

    while (accept(ps, ",")) {
        n = eat_init_declarator_2(ps, dsseq);
        list.as<ast::init_declarator_list>()->push_back(std::move(n));
    }
    return list;
}

