#include "parse.hpp"


ast_node eat_literal_2(parse_state& ps) {
    token tok;
    if (eat_token(ps, tt_literal, &tok)) {
        return ast_node::make<ast::literal>(tok);
    }
    if (eat_token(ps, tt_string_literal, &tok)) {
        return ast_node::make<ast::literal>(tok);
    }
    if (eat_token(ps, kw_nullptr, &tok)) {
        return ast_node::make<ast::nullptr_literal>(tok);
    }
    if (eat_token(ps, kw_true, &tok)) {
        return ast_node::make<ast::boolean_literal>(tok);
    }
    if (eat_token(ps, kw_false, &tok)) {
        return ast_node::make<ast::boolean_literal>(tok);
    }
    return ast_node::null();
}

ast_node eat_this_2(parse_state& ps) {
    if (eat_keyword(ps, kw_this)) {
        return ast_node::make<ast::this_>();
    }
    return ast_node::null();
}

ast_node eat_primary_expression_2(parse_state& ps) {
    ast_node n = ast_node::null();
    n = eat_literal_2(ps);
    if(n) return n;

    n = eat_this_2(ps);
    if(n) return n;

    if (accept(ps, "(")) {
        {
            CONTEXT_SCOPE(e_ctx_normal);
            n = eat_expression_2(ps);
        }
        expect(ps, ")");
        return n;
    }

    n = eat_id_expression_2(ps);
    if(n) return n;

    // TODO: lambda-expression

    return ast_node::null();
}

ast_node eat_unqualified_id_2(parse_state& ps) {
    ast_node n = ast_node::null();
    n = eat_template_id_2(ps);
    if(n) return n;

    {
        REWIND_SCOPE(ident);
        token tok;
        if (eat_token(ps, tt_identifier, &tok)) {
            symbol_ref sym = nullptr;
            if (ps.get_current_context() == e_ctx_declarator_name) {
                int lookup_flags
                    = LOOKUP_FLAG_OBJECT
                    | LOOKUP_FLAG_FUNCTION
                    | LOOKUP_FLAG_TYPEDEF;
                sym = ps.get_current_scope()->get_symbol(tok.str, lookup_flags);
            } else {
                int lookup_flags
                    = LOOKUP_FLAG_CLASS
                    | LOOKUP_FLAG_ENUM
                    | LOOKUP_FLAG_TYPEDEF
                    | LOOKUP_FLAG_ENUMERATOR
                    | LOOKUP_FLAG_FUNCTION
                    | LOOKUP_FLAG_OBJECT
                    | LOOKUP_FLAG_TEMPLATE_PARAMETER;
                sym = ps.get_current_scope()->lookup(tok.str, lookup_flags);
            }
            if (sym) {
                if (sym->as<symbol_class>()) {
                    return ast_node::make<ast::class_name>(sym);
                }
                if (sym->as<symbol_enum>()) {
                    return ast_node::make<ast::enum_name>(sym);
                }
                if (sym->as<symbol_typedef>()) {
                    return ast_node::make<ast::typedef_name>(sym);
                }
                if (sym->as<symbol_enumerator>()) {
                    return ast_node::make<ast::enumerator_name>(sym);
                }
                if (sym->as<symbol_function>()) {
                    return ast_node::make<ast::function_name>(sym);
                }
                if (sym->as<symbol_object>()) {
                    return ast_node::make<ast::object_name>(sym);
                }
                if (auto stp = sym->as<symbol_template_parameter>()) {
                    if (stp->kind == e_template_parameter_type) {
                        return ast_node::make<ast::dependent_type_name>(sym);
                    } else if (stp->kind == e_template_parameter_non_type) {
                        return ast_node::make<ast::dependent_non_type_name>(sym);
                    } else {
                        throw parse_exception("TODO: unsupported template parameter kind", ps.get_latest_token());
                    }
                }
            }
            return ast_node::make<ast::identifier>(tok);
            // Unreachable, for now?
            REWIND_ON_EXIT(ident);
        }
        /*
        n = eat_identifier_2(ps);
        if(n) return n;*/
    }

    n = eat_operator_function_id_2(ps);
    if(n) return n;

    n = eat_conversion_function_id_2(ps);
    if(n) return n;

    n = eat_literal_operator_id_2(ps);
    if(n) return n;

    {
        REWIND_SCOPE(destructor_name);
        if (accept(ps, "~")) {
            // TODO: mark as destructor
            ast_node n = eat_class_name_2(ps);
            if(n) return n;
            n = eat_decltype_specifier_2(ps);
            if(n) return n;
            REWIND_ON_EXIT(destructor_name);
        }
    }

    return ast_node::null();
}

ast_node eat_qualified_id_2(parse_state& ps) {
    ast_node nns = eat_nested_name_specifier_2(ps);
    if (!nns) {
        return ast_node::null();
    }

    if (accept(ps, kw_template)) {
        // TODO: note this
    }

    ast_node unq = eat_unqualified_id_2(ps);
    if (!unq) {
        throw parse_exception("Expected an unqualified-id", ps.peek_token());
    }
    nns.as<ast::nested_name_specifier>()->push_next(std::move(unq));
    return nns;
}

ast_node eat_id_expression_2(parse_state& ps) {
    ast_node n = ast_node::null();

    n = eat_qualified_id_2(ps);
    if(n) return n;

    n = eat_unqualified_id_2(ps);
    if(n) return n;

    return ast_node::null();
}

ast_node eat_nested_name_specifier_opener_syn(parse_state& ps) {    
    if (accept(ps, "::")) {
        return ast_node::make<ast::nested_name_specifier>();
    }

    ast_node n = ast_node::null();

    REWIND_SCOPE(nested_name_specifier_part);
    n = eat_type_name_2(ps);
    if(n) {
        if (!accept(ps, "::")) {
            REWIND_ON_EXIT(nested_name_specifier_part);
            return ast_node::null();
        }
        return ast_node::make<ast::nested_name_specifier>(std::move(n));
    }

    n = eat_decltype_specifier_2(ps);
    if (n) {
        if (!accept(ps, "::")) {
            REWIND_ON_EXIT(nested_name_specifier_part);
            return ast_node::null();
        }
        return ast_node::make<ast::nested_name_specifier>(std::move(n));
    }

    return ast_node::null();
}

ast_node eat_nested_name_specifier_syn(parse_state& ps) {
    ast_node nns = eat_nested_name_specifier_opener_syn(ps);
    if (!nns) {
        return ast_node::null();
    }

    ast::nested_name_specifier* nns_cur = nns.as<ast::nested_name_specifier>();
    while (true) {
        REWIND_SCOPE(nested_name_specifier_part);
        ast_node n = ast_node::null();
        if (accept(ps, kw_template)) {
            n = eat_simple_template_id_2(ps);
        } else {
            n = eat_simple_template_id_2(ps);
            if (!n) {
                n = eat_identifier_2(ps);
            }
        }
        if (!n) {
            return nns;
        }
        if (!accept(ps, "::")) {
            REWIND_ON_EXIT(nested_name_specifier_part);
            break;
        }

        auto nns_next = ast_node::make<ast::nested_name_specifier>(std::move(n));
        auto tmp = nns_next.as<ast::nested_name_specifier>();
        nns_cur->set_next(std::move(nns_next));
        nns_cur = tmp;
    }
    return nns;
}

ast_node eat_nested_name_specifier_opener_sem(parse_state& ps) {    
    if (accept(ps, "::")) {
        return ast_node::make<ast::nested_name_specifier>();
    }

    ast_node n = ast_node::null();

    REWIND_SCOPE(nested_name_specifier_part);
    n = eat_type_name_sem(ps);
    if(n) {
        if (!accept(ps, "::")) {
            REWIND_ON_EXIT(nested_name_specifier_part);
            return ast_node::null();
        }
        return ast_node::make<ast::nested_name_specifier>(std::move(n));
    }

    n = eat_namespace_name_sem(ps);
    if (n) {
        if (!accept(ps, "::")) {
            REWIND_ON_EXIT(nested_name_specifier_part);
            return ast_node::null();
        }
        return ast_node::make<ast::nested_name_specifier>(std::move(n));
    }

    n = eat_decltype_specifier_2(ps);
    if (n) {
        if (!accept(ps, "::")) {
            REWIND_ON_EXIT(nested_name_specifier_part);
            return ast_node::null();
        }
        return ast_node::make<ast::nested_name_specifier>(std::move(n));
    }

    return ast_node::null();
}
void eat_nested_name_specifier_sem_dependent(parse_state& ps, ast::nested_name_specifier* nns_cur) {
    while (true) {        
        REWIND_SCOPE(nested_name_specifier_part);
        ast_node n = ast_node::null();
        if (accept(ps, kw_template)) {
            n = eat_simple_template_id_dependent(ps);
        } else {
            // TODO: Only consume simple_template_id if context allows
            n = eat_simple_template_id_dependent(ps);
            if (!n) {
                n = eat_identifier_2(ps);
            }
        }
        if (!n) {
            REWIND_ON_EXIT(nested_name_specifier_part);
            return;
        }
        if (!accept(ps, "::")) {
            REWIND_ON_EXIT(nested_name_specifier_part);
            break;
        }

        auto nns_next = ast_node::make<ast::nested_name_specifier>(std::move(n));
        auto tmp = nns_next.as<ast::nested_name_specifier>();
        nns_cur->set_next(std::move(nns_next));
        nns_cur = tmp;
    }
}
ast_node eat_nested_name_specifier_sem(parse_state& ps) {
    ast_node nns = eat_nested_name_specifier_opener_sem(ps);
    if (!nns) {
        return ast_node::null();
    }

    bool dependent_mode = nns.is_dependent();

    ast::nested_name_specifier* nns_cur = nns.as<ast::nested_name_specifier>();
    while (true) {
        if (dependent_mode) {
            eat_nested_name_specifier_sem_dependent(ps, nns_cur);
            return nns;
        }

        symbol_ref cur_sym = nullptr;
        symbol_table_ref qualified_scope = nullptr;
        if (!nns_cur->n) {
            qualified_scope = ps.get_current_scope();
            while (qualified_scope->get_enclosing()) {
                qualified_scope = qualified_scope->get_enclosing();
            }
        } else if (ast::class_name* class_name = nns_cur->n.as<ast::class_name>()) {
            cur_sym = class_name->resolve_to_symbol(ps);
            if (!cur_sym->is_defined()) {
                throw parse_exception("nested-name-specifier: type is incomplete", ps.get_latest_token());
            }
            qualified_scope = cur_sym->nested_symbol_table;
        } else if (ast::enum_name* enum_name = nns_cur->n.as<ast::enum_name>()) {
            cur_sym = enum_name->resolve_to_symbol(ps);
            if (!cur_sym->is_defined()) {
                throw parse_exception("nested-name-specifier: type is incomplete", ps.get_latest_token());
            }
            qualified_scope = cur_sym->nested_symbol_table;
        } else if (ast::namespace_name* namespace_name = nns_cur->n.as<ast::namespace_name>()) {
            cur_sym = namespace_name->sym;
            if (!cur_sym->is_defined()) {
                throw parse_exception("nested-name-specifier: type is incomplete", ps.get_latest_token());
            }
            qualified_scope = cur_sym->nested_symbol_table;
        } else if (ast::simple_template_id* stid = nns_cur->n.as<ast::simple_template_id>()) {
            cur_sym = stid->resolve_to_symbol(ps);
            if (!cur_sym->is_defined()) {
                throw parse_exception("nested-name-specifier: type is incomplete", ps.get_latest_token());
            }
            qualified_scope = cur_sym->nested_symbol_table;
        }

        if (!qualified_scope) {
            throw parse_exception("nested-name-specifier failed to resolve to a scope", ps.get_latest_token());
        }

        REWIND_SCOPE(nested_name_specifier_part);
        ast_node n = ast_node::null();
        if (accept(ps, kw_template)) {
            n = eat_simple_template_id_2(ps, qualified_scope);
        } else {
            n = eat_type_name_sem(ps, qualified_scope);
            if (!n) {
                n = eat_namespace_name_sem(ps, qualified_scope);
            }
            /*
            n = eat_simple_template_id_2(ps, qualified_scope);
            if (!n) {
                n = eat_identifier_2(ps);
            }*/
        }
        if (!n) {
            return nns;
        }
        if (!accept(ps, "::")) {
            REWIND_ON_EXIT(nested_name_specifier_part);
            break;
        }

        if (n.is_dependent()) {
            dependent_mode = true;
        }

        auto nns_next = ast_node::make<ast::nested_name_specifier>(std::move(n));
        auto tmp = nns_next.as<ast::nested_name_specifier>();
        nns_cur->set_next(std::move(nns_next));
        nns_cur = tmp;
    }
    return nns;
}

ast_node eat_nested_name_specifier_2(parse_state& ps) {
    if (ps.is_semantic()) {
        return eat_nested_name_specifier_sem(ps);
    } else {
        return eat_nested_name_specifier_syn(ps);
    }
}

ast_node eat_sizeof_expr_2(parse_state& ps) {
    if (accept(ps, kw_sizeof)) {
        if (accept(ps, "...")) {
            expect(ps, "(");
            ast_node ident = eat_identifier_2(ps);
            expect(ps, ")");
            return ast_node::make<ast::size_of>(std::move(ident), true);
        } else if(accept(ps, "(")) {
            ast_node tid = eat_type_id_2(ps);
            expect(ps, ")");
            return ast_node::make<ast::size_of>(std::move(tid));
        } else {
            ast_node expr = eat_unary_expression_2(ps);
            if(!expr) throw parse_exception("sizeof: expected an expression or '('", ps.peek_token());
            return ast_node::make<ast::size_of>(std::move(expr));
        }
    } else if (accept(ps, kw_alignof)) {
        expect(ps, "(");
        ast_node tid = eat_type_id_2(ps);
        expect(ps, ")");

        return ast_node::make<ast::align_of>(std::move(tid));
    }
    return ast_node::null();
}

ast_node eat_noexcept_expr_2(parse_state& ps) {
    if (!accept(ps, kw_noexcept)) {
        return ast_node::null();
    }
    expect(ps, "(");
    ast_node expr = eat_expression_2(ps);
    expect(ps, ")");
    if(!expr) throw parse_exception("noexcept requires an expression", ps.peek_token());
    return ast_node::make<ast::expr_noexcept>(std::move(expr));
}

ast_node eat_new_expr_2(parse_state& ps) {
    /*
    REWIND_SCOPE(new_expr);
    bool is_scoped = false;
    bool is_array = false;
    if (accept(ps, "::")) {
        is_scoped = true;
    }
    if (!accept(ps, kw_new)) {
        REWIND_ON_EXIT(new_expr);
        return ast_node::null();
    }

    ast_node placement = eat_new_placement_2(ps);
    if (accept(ps, "(")) {
        ast_node tid = eat_type_id_2(ps);
        expect(ps, ")");
    } else {
        ast_node ntid = eat_new_type_id_2(ps);
    }
    */
    // TODO: implement new-placement and new-type-id
    return ast_node::null();
}

ast_node eat_delete_expr_2(parse_state& ps) {
    REWIND_SCOPE(delete_expr);
    bool is_scoped = false;
    bool is_array = false;
    if (accept(ps, "::")) {
        is_scoped = true;
    }

    if (!accept(ps, kw_delete)) {
        REWIND_ON_EXIT(delete_expr);
        return ast_node::null();
    }

    if (accept(ps, "[")) {
        expect(ps, "]");
        is_array = true;
    }

    ast_node expr = eat_cast_expression_2(ps);
    if(!expr) throw parse_exception("delete requires a cast-expression", ps.peek_token());

    return ast_node::make<ast::expr_delete>(std::move(expr), is_array, is_scoped);
}

ast_node eat_pseudo_destructor_name_2(parse_state& ps) {
    REWIND_SCOPE(pseudo_destructor_name);
    if (accept(ps, "~")) {
        ast_node decltype_spec = eat_decltype_specifier_2(ps);
        if(!decltype_spec) throw parse_exception("Expected a decltype-specifier", ps.peek_token());
        return ast_node::make<ast::pseudo_destructor_name>(std::move(decltype_spec));
    }

    ast_node nns = eat_nested_name_specifier_2(ps);
    if (nns) {
        if (accept(ps, kw_template)) {
            ast_node stid = eat_simple_template_id_2(ps);
            if(!stid) throw parse_exception("pseudo-destructor-name expected a simple-template-specifier", ps.peek_token());
            expect(ps, "::");
            expect(ps, "~");
            ast_node type_name = eat_type_name_2(ps);
            if(!type_name) throw parse_exception("pseudo-destructor-name expected a type-name", ps.peek_token());
            nns.as<ast::nested_name_specifier>()->push_next(std::move(stid));
            nns.as<ast::nested_name_specifier>()->push_next(std::move(type_name));
            return ast_node::make<ast::pseudo_destructor_name>(std::move(nns));
        }
    }

    if (!accept(ps, "~")) {
        REWIND_ON_EXIT(pseudo_destructor_name);
        return ast_node::null();
    }

    ast_node type_name = eat_type_name_2(ps);
    if(!type_name) throw parse_exception("pseudo-destructor-name expected a type-name", ps.peek_token());

    if (nns) {
        nns.as<ast::nested_name_specifier>()->push_next(std::move(type_name));
        return ast_node::make<ast::pseudo_destructor_name>(std::move(nns));
    } else {
        return ast_node::make<ast::pseudo_destructor_name>(std::move(type_name));
    }

    // Unreachable
    return ast_node::null();
}

ast_node eat_postfix_expr_suffix_2(parse_state& ps, ast_node&& wrapped) {
    REWIND_SCOPE(postfix_expr_suffix);
    
    if (accept(ps, "[")) {
        // postfix-expression [ expression ]
        // postfix-expression [ braced-init-list ]
        ast_node subscript = eat_expression_2(ps);
        if (!subscript) {
            subscript = eat_braced_init_list_2(ps);
        }
        expect(ps, "]");
        if(!subscript) throw parse_exception("Array subscript expected an expression or braced-init-list", ps.peek_token());
        wrapped = ast_node::make<ast::expr_array_subscript>(std::move(wrapped), std::move(subscript));
    } else if (accept(ps, "(")) {
        // postfix-expression ( expression-listopt )
        ast_node args = eat_expression_list_2(ps);
        expect(ps, ")");
        wrapped = ast_node::make<ast::expr_fn_call>(std::move(wrapped), std::move(args));
    } else if (accept(ps, ".")) {
        // postfix-expression . templateopt id-expression
        // postfix-expression . pseudo-destructor-name
        bool is_tpl = false;
        if (accept(ps, kw_template)) { is_tpl = true; }
        ast_node n = eat_id_expression_2(ps);
        if (!n && !is_tpl) {
            n = eat_pseudo_destructor_name_2(ps);
        }
        if(!n) throw parse_exception("Member access expected an id-expression or pseudo-destructor-name", ps.peek_token());
        wrapped = ast_node::make<ast::expr_member_access>(std::move(wrapped), std::move(n));
    } else if (accept(ps, "->")) {
        // postfix-expression -> templateopt id-expression
        // postfix-expression -> pseudo-destructor-name
        bool is_tpl = false;
        if(accept(ps, kw_template)) { is_tpl = true; }
        ast_node n = eat_id_expression_2(ps);
        if (!n && !is_tpl) {
            n = eat_pseudo_destructor_name_2(ps);
        }
        if(!n) throw parse_exception("Pointer member access (->) expected an id-expression or pseudo-destructor-name", ps.peek_token());
        wrapped = ast_node::make<ast::expr_ptr_member_access>(std::move(wrapped), std::move(n));
    } else if (accept(ps, "++")) {
        // postfix-expression ++
        wrapped = ast_node::make<ast::expr_unary>(e_unary_incr_post, std::move(wrapped));
    } else if (accept(ps, "--")) {
        // postfix-expression --
        wrapped = ast_node::make<ast::expr_unary>(e_unary_decr_post, std::move(wrapped));
    } else {
        return std::move(wrapped);
    }

    return eat_postfix_expr_suffix_2(ps, std::move(wrapped));
}

ast_node eat_postfix_expr_2(parse_state& ps) {
    REWIND_SCOPE(postfix_expr);
    ast_node n = ast_node::null();
    
    decl_flags flags;
    // primary-expression
    if (n = eat_primary_expression_2(ps)) {

    } else if (n = eat_simple_type_specifier_2(ps, flags)) {
        // simple-type-specifier ( expression-listopt )
        // simple-type-specifier braced-init-list
        ast_node inner = ast_node::null();
        if (accept(ps, "(")) {
            inner = eat_expression_list_2(ps);
            expect(ps, ")");
            return ast_node::make<ast::expr_postfix>(std::move(n), std::move(inner));
        } else {
            inner = eat_braced_init_list_2(ps);
            if (!inner) {
                REWIND_ON_EXIT(postfix_expr);
                return ast_node::null();
            }
            return ast_node::make<ast::expr_postfix>(std::move(n), std::move(inner));
        }
    } else if (n = eat_typename_specifier_2(ps)) {
        // typename-specifier ( expression-listopt )
        // typename-specifier braced-init-list
        ast_node inner = ast_node::null();
        if (accept(ps, "(")) {
            inner = eat_expression_list_2(ps);
            expect(ps, ")");
            return ast_node::make<ast::expr_postfix>(std::move(n), std::move(inner));
        } else {
            inner = eat_braced_init_list_2(ps);
            if (!inner) {
                REWIND_ON_EXIT(postfix_expr);
                return ast_node::null();
            }
            return ast_node::make<ast::expr_postfix>(std::move(n), std::move(inner));
        }
    } else if (accept(ps, kw_dynamic_cast)) {
        // dynamic_cast < type-id > ( expression )
        expect(ps, "<");
        ast_node tid = eat_type_id_2(ps);
        expect(ps, ">");
        expect(ps, "(");
        ast_node expr = eat_expression_2(ps);
        expect(ps, ")");
        n = ast_node::make<ast::expr_cast>(std::move(tid), std::move(expr), e_cast_dynamic);
    } else if (accept(ps, kw_static_cast)) {
        // static_cast < type-id > ( expression )
        expect(ps, "<");
        ast_node tid = eat_type_id_2(ps);
        expect(ps, ">");
        expect(ps, "(");
        ast_node expr = eat_expression_2(ps);
        expect(ps, ")");
        n = ast_node::make<ast::expr_cast>(std::move(tid), std::move(expr), e_cast_static);
    } else if (accept(ps, kw_reinterpret_cast)) {
        // reinterpret_cast < type-id > ( expression )
        expect(ps, "<");
        ast_node tid = eat_type_id_2(ps);
        expect(ps, ">");
        expect(ps, "(");
        ast_node expr = eat_expression_2(ps);
        expect(ps, ")");
        n = ast_node::make<ast::expr_cast>(std::move(tid), std::move(expr), e_cast_reinterpret);
    } else if (accept(ps, kw_const_cast)) {
        // const_cast < type-id > ( expression )
        expect(ps, "<");
        ast_node tid = eat_type_id_2(ps);
        expect(ps, ">");
        expect(ps, "(");
        ast_node expr = eat_expression_2(ps);
        expect(ps, ")");
        n = ast_node::make<ast::expr_cast>(std::move(tid), std::move(expr), e_cast_const);
    } else if (accept(ps, kw_typeid)) {
        // typeid ( expression )
        // typeid ( type-id )
        expect(ps, "(");
        ast_node arg = eat_expression_2(ps);
        if (!arg) {
            arg = eat_type_id_2(ps);
        }
        expect(ps, ")");
        if(!arg) throw parse_exception("typeid requires an expression or type-id as argument", ps.peek_token());
        n = ast_node::make<ast::expr_typeid>(std::move(arg));
    } else {
        return ast_node::null();
    }
    
    return eat_postfix_expr_suffix_2(ps, std::move(n));
}

ast_node eat_unary_expression_2(parse_state& ps) {
    ast_node n = ast_node::null();
    n = eat_postfix_expr_2(ps);
    if(n) return n;

    if (accept(ps, "++")) {
        ast_node e = eat_cast_expression_2(ps);
        if(!e) throw parse_exception("expected an expression after '++'", ps.peek_token());
        return ast_node::make<ast::expr_unary>(e_unary_incr_pre, std::move(e));
    }
    if (accept(ps, "--")) {
        ast_node e = eat_cast_expression_2(ps);
        if(!e) throw parse_exception("expected an expression after '--'", ps.peek_token());
        return ast_node::make<ast::expr_unary>(e_unary_decr_pre, std::move(e));
    }

    if (accept(ps, "*")) {
        ast_node e = eat_cast_expression_2(ps);
        if(!e) throw parse_exception("expected an expression after '*'", ps.peek_token());
        return ast_node::make<ast::expr_deref>(std::move(e));
    }
    if (accept(ps, "&")) {
        ast_node e = eat_cast_expression_2(ps);
        if(!e) throw parse_exception("expected an expression after '&'", ps.peek_token());
        return ast_node::make<ast::expr_amp>(std::move(e));
    }
    if (accept(ps, "+")) {
        ast_node e = eat_cast_expression_2(ps);
        if(!e) throw parse_exception("expected an expression after '+'", ps.peek_token());
        return ast_node::make<ast::expr_plus>(std::move(e));
    }
    if (accept(ps, "-")) {
        ast_node e = eat_cast_expression_2(ps);
        if(!e) throw parse_exception("expected an expression after '-'", ps.peek_token());
        return ast_node::make<ast::expr_minus>(std::move(e));
    }
    if (accept(ps, "!")) {
        ast_node e = eat_cast_expression_2(ps);
        if(!e) throw parse_exception("expected an expression after '!'", ps.peek_token());
        return ast_node::make<ast::expr_lnot>(std::move(e));
    }
    if (accept(ps, "~")) {
        ast_node e = eat_cast_expression_2(ps);
        if(!e) throw parse_exception("expected an expression after '~'", ps.peek_token());
        return ast_node::make<ast::expr_not>(std::move(e));
    }

    n = eat_sizeof_expr_2(ps);
    if(n) return n;

    n = eat_noexcept_expr_2(ps);
    if(n) return n;

    n = eat_new_expr_2(ps);
    if(n) return n;

    n = eat_delete_expr_2(ps);
    if(n) return n;

    return ast_node::null();
}

ast_node eat_cast_expression_2(parse_state& ps) {
    {
        REWIND_SCOPE(cast_expression);
        if (accept(ps, "(")) {
            ast_node expr = ast_node::null();
            ast_node type_id = eat_type_id_2(ps);
            if (!type_id) {
                REWIND_ON_EXIT(cast_expression);
                goto no_cast;
            }
            expect(ps, ")");
            expr = eat_cast_expression_2(ps);
            if (!expr) {
                throw parse_exception("Expected an expression", ps.peek_token());
            }
            return ast_node::make<ast::expr_cast>(std::move(type_id), std::move(expr));
        }
    }
no_cast:
    return eat_unary_expression_2(ps);
}

ast_node eat_pm_expression_2(parse_state& ps) {
    ast_node left = eat_cast_expression_2(ps);
    if(!left) return ast_node::null();
    REWIND_SCOPE(pm_expression_2);

    while (accept(ps, ".*") || accept(ps, "->*")) {
        ast_node right = eat_cast_expression_2(ps);
        if(!right) { REWIND_ON_EXIT(pm_expression_2); return left; };
        left = ast_node::make<ast::expr_pm>(std::move(left), std::move(right));
    }
    return left;
}

bool eat_multiplicative_op_2(parse_state& ps, e_operator& op) {
    op = e_operator_none;
    if (accept(ps, "*")) {
        op = e_mul;
    } else if (accept(ps, "/")) {
        op = e_div;
    } else if (accept(ps, "%")) {
        op = e_mod;
    }
    return op != e_operator_none;
}
ast_node eat_multiplicative_expression_2(parse_state& ps) {
    ast_node left = eat_pm_expression_2(ps);
    if(!left) return ast_node::null();
    REWIND_SCOPE(multiplicative_expression_2);

    e_operator op;
    while (eat_multiplicative_op_2(ps, op)) {
        ast_node right = eat_pm_expression_2(ps);
        if(!right) { REWIND_ON_EXIT(multiplicative_expression_2); return left; };
        left = ast_node::make<ast::expr_mul>(op, std::move(left), std::move(right));
    }
    return left;
}

bool eat_additive_op_2(parse_state& ps, e_operator& op) {
    op = e_operator_none;
    if (accept(ps, "+")) {
        op = e_add;
    } else if (accept(ps, "-")) {
        op = e_sub;
    }
    return op != e_operator_none;
}
ast_node eat_additive_expression_2(parse_state& ps) {
    ast_node left = eat_multiplicative_expression_2(ps);
    if(!left) return ast_node::null();
    REWIND_SCOPE(additive_expression_2);

    e_operator op;
    while (eat_additive_op_2(ps, op)) {
        ast_node right = eat_multiplicative_expression_2(ps);
        if(!right) { REWIND_ON_EXIT(additive_expression_2); return left; };
        left = ast_node::make<ast::expr_add>(op, std::move(left), std::move(right));
    }
    return left;
}

bool eat_shift_op_2(parse_state& ps, e_operator& op) {
    op = e_operator_none;
    if (accept(ps, "<<")) {
        op = e_lshift;
    } else if (ps.get_current_context() != e_ctx_template_argument_list && accept(ps, ">>")) {
        op = e_rshift;
    }
    return op != e_operator_none;
}
ast_node eat_shift_expression_2(parse_state& ps) {
    ast_node left = eat_additive_expression_2(ps);
    if(!left) return ast_node::null();
    REWIND_SCOPE(shift_expression_2);

    e_operator op;
    while (eat_shift_op_2(ps, op)) {
        ast_node right = eat_additive_expression_2(ps);
        if(!right) { REWIND_ON_EXIT(shift_expression_2); return left; };
        left = ast_node::make<ast::expr_shift>(op, std::move(left), std::move(right));
    }
    return left;
}

bool eat_relational_op_2(parse_state& ps, e_operator& op) {
    op = e_operator_none;
    if (accept(ps, "<")) {
        op = e_less;
    } else if (ps.get_current_context() != e_ctx_template_argument_list && accept(ps, ">")) {
        op = e_more;
    } else if (accept(ps, "<=")) {
        op = e_lesseq;
    } else if (accept(ps, ">=")) {
        op = e_moreeq;
    }
    return op != e_operator_none;
}
ast_node eat_relational_expression_2(parse_state& ps) {
    ast_node left = eat_shift_expression_2(ps);
    if(!left) return ast_node::null();
    REWIND_SCOPE(relational_expression_2);

    e_operator op;
    while (eat_relational_op_2(ps, op)) {
        ast_node right = eat_shift_expression_2(ps);
        if(!right) { REWIND_ON_EXIT(relational_expression_2); return left; };
        left = ast_node::make<ast::expr_rel>(op, std::move(left), std::move(right));
    }
    return left;
}

bool eat_equality_op_2(parse_state& ps, e_operator& op) {
    op = e_operator_none;
    if (accept(ps, "==")) {
        op = e_equal;
    } else if (accept(ps, "!=")) {
        op = e_notequal;
    }
    return op != e_operator_none;
}
ast_node eat_equality_expression_2(parse_state& ps) {
    ast_node left = eat_relational_expression_2(ps);
    if(!left) return ast_node::null();
    REWIND_SCOPE(equality_expression_2);

    e_operator op;
    while (eat_equality_op_2(ps, op)) {
        ast_node right = eat_relational_expression_2(ps);
        if(!right) { REWIND_ON_EXIT(equality_expression_2); return left; };
        left = ast_node::make<ast::expr_eq>(op, std::move(left), std::move(right));
    }
    return left;
}

ast_node eat_and_expression_2(parse_state& ps) {
    ast_node left = eat_equality_expression_2(ps);
    if(!left) return ast_node::null();
    REWIND_SCOPE(and_expression_2);


    while (accept(ps, "&")) {
        ast_node right = eat_equality_expression_2(ps);
        if(!right) { REWIND_ON_EXIT(and_expression_2); return left; };
        left = ast_node::make<ast::expr_and>(std::move(left), std::move(right));
    }
    return left;
}

ast_node eat_exclusive_or_expression_2(parse_state& ps) {
    ast_node left = eat_and_expression_2(ps);
    if(!left) return ast_node::null();
    REWIND_SCOPE(exclusive_or_expression_2);

    while (accept(ps, "^")) {
        ast_node right = eat_and_expression_2(ps);
        if(!right) { REWIND_ON_EXIT(exclusive_or_expression_2); return left; };
        left = ast_node::make<ast::expr_exor>(std::move(left), std::move(right));
    }
    return left;
}

ast_node eat_inclusive_or_expression_2(parse_state& ps) {
    ast_node left = eat_exclusive_or_expression_2(ps);
    if(!left) return ast_node::null();
    REWIND_SCOPE(inclusive_or_expression_2);

    while (accept(ps, "|")) {
        ast_node right = eat_exclusive_or_expression_2(ps);
        if(!right) { REWIND_ON_EXIT(inclusive_or_expression_2); return left; };
        left = ast_node::make<ast::expr_ior>(std::move(left), std::move(right));
    }
    return left;
}

ast_node eat_logical_and_expression_2(parse_state& ps) {
    ast_node left = eat_inclusive_or_expression_2(ps);
    if(!left) return ast_node::null();
    REWIND_SCOPE(logical_and_expression_2);

    while (accept(ps, "&&")) {
        ast_node right = eat_inclusive_or_expression_2(ps);
        if(!right) { REWIND_ON_EXIT(logical_and_expression_2); return left; };
        left = ast_node::make<ast::expr_land>(std::move(left), std::move(right));
    }
    return left;
}

ast_node eat_logical_or_expression_2(parse_state& ps) {
    ast_node left = eat_logical_and_expression_2(ps);
    if(!left) return ast_node::null();
    REWIND_SCOPE(logical_or_expression_2);

    while (accept(ps, "||")) {
        ast_node right = eat_logical_and_expression_2(ps);
        if(!right) { REWIND_ON_EXIT(logical_or_expression_2); return left; };
        left = ast_node::make<ast::expr_lor>(std::move(left), std::move(right));
    }
    return left;
}

ast_node eat_conditional_expression_2(parse_state& ps) {
    ast_node n = ast_node::null();
    if (!(n = eat_logical_or_expression_2(ps))) {
        return ast_node::null();
    }
    if (!accept(ps, "?")) {
        return n;
    }
    
    ast_node a = eat_expression_2(ps);
    if (!a) {
        throw parse_exception("Expected an expression", ps.peek_token());
    }
    expect(ps, ":");
    ast_node b = eat_assignment_expression_2(ps);
    if (!b) {
        throw parse_exception("Expected an expression", ps.peek_token());
    }
    // logical-or-expression ? expression : assignment-expression
    return ast_node::make<ast::expr_cond>(std::move(n), std::move(a), std::move(b));
}

ast_node eat_assignment_expression_2(parse_state& ps) {
    ast_node n = eat_conditional_expression_2(ps);
    if (n.is<ast::expr_cond>()) {
        return n;
    }
    if (!n) {
        return ast_node::null();
    }
    REWIND_SCOPE(assignment_expression_2);

    if (accept(ps, "=") ||
        accept(ps, "*=") ||
        accept(ps, "/=") ||
        accept(ps, "%=") ||
        accept(ps, "+=") ||
        accept(ps, "-=") ||
        accept(ps, ">>=") ||
        accept(ps, "<<=") ||
        accept(ps, "&=") ||
        accept(ps, "^=") ||
        accept(ps, "|=")
    ) {
        ast_node right = eat_initializer_clause_2(ps);
        if(!right) { REWIND_ON_EXIT(assignment_expression_2); return n; };
        n = ast_node::make<ast::expr_assign>(std::move(n), std::move(right));
    }
    return n;
}

ast_node eat_expression_2(parse_state& ps) {
    ast_node n = eat_assignment_expression_2(ps);
    while (accept(ps, ",")) {
        ast_node n = eat_assignment_expression_2(ps);
        if (!n) {
            throw parse_exception("Expected an expression", ps.peek_token());
        }
    }
    return n;
}

ast_node eat_expression_list_2(parse_state& ps) {
    return eat_initializer_list_2(ps);
}

ast_node eat_constant_expression_2(parse_state& ps) {
    return eat_conditional_expression_2(ps);
}

