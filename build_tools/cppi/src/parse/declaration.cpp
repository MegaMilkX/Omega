#include "parse.hpp"
#include "decl_util.hpp"


ast_node eat_elaborated_type_specifier_class_syn(parse_state& ps) {
    REWIND_SCOPE(elaborated_type_specifier_class);
    e_class_key key;
    if (!eat_class_key_2(ps, key)) {
        return ast_node::null();
    }

    bool has_attribs = eat_attribute_specifier_seq(ps);
    if (has_attribs) {
        ast_node nns = eat_nested_name_specifier_2(ps);
        ast_node ident = eat_identifier_2(ps);
        if (nns && ident) {
            nns.as<ast::nested_name_specifier>()->push_next(std::move(ident));
            return ast_node::make<ast::elaborated_type_specifier>(key, std::move(nns), nullptr);
        } else if(ident) {
            return ast_node::make<ast::elaborated_type_specifier>(key, std::move(ident), nullptr);
        }
        REWIND_ON_EXIT(elaborated_type_specifier_class);
        return ast_node::null();
    }

    ast_node nns = eat_nested_name_specifier_2(ps);
    if (nns) {
        bool is_tpl = accept(ps, kw_template);
        ast_node stid = eat_simple_template_id_2(ps);
        if (!stid && is_tpl) {
            throw parse_exception("Expected a simple-template-id", ps.peek_token());
        }
        if (stid) {
            nns.as<ast::nested_name_specifier>()->push_next(std::move(stid));
            return ast_node::make<ast::elaborated_type_specifier>(key, std::move(nns), nullptr);
        }
        ast_node ident = eat_identifier_2(ps);
        if (ident) {
            nns.as<ast::nested_name_specifier>()->push_next(std::move(ident));
            return ast_node::make<ast::elaborated_type_specifier>(key, std::move(nns), nullptr);
        }
        REWIND_ON_EXIT(elaborated_type_specifier_class);
        return ast_node::null();
    }

    ast_node stid = eat_simple_template_id_2(ps);
    if (stid) {
        return ast_node::make<ast::elaborated_type_specifier>(key, std::move(stid), nullptr);
    }
    ast_node ident = eat_identifier_2(ps);
    if (ident) {
        return ast_node::make<ast::elaborated_type_specifier>(key, std::move(ident), nullptr);
    }

    REWIND_ON_EXIT(elaborated_type_specifier_class);
    return ast_node::null();
}

ast_node eat_elaborated_type_specifier_enum_syn(parse_state& ps) {
    REWIND_SCOPE(elaborated_type_specifier_enum);
    if (!eat_keyword(ps, kw_enum)) {
        return ast_node::null();
    }

    ast_node nns = eat_nested_name_specifier_2(ps);
    ast_node ident = eat_identifier_2(ps);
    if (!ident) {
        REWIND_ON_EXIT(elaborated_type_specifier_enum);
        return ast_node::null();
    }

    if (nns) {
        nns.as<ast::nested_name_specifier>()->push_next(std::move(ident));
        return ast_node::make<ast::elaborated_type_specifier>(ck_enum, std::move(nns), nullptr);
    } else {
        return ast_node::make<ast::elaborated_type_specifier>(ck_enum, std::move(ident), nullptr);
    }
}

ast_node eat_elaborated_type_specifier_syn(parse_state& ps) {
    ast_node n = ast_node::null();

    n = eat_elaborated_type_specifier_class_syn(ps);
    if(n) return n;

    n = eat_elaborated_type_specifier_enum_syn(ps);
    if(n) return n;

    return ast_node::null();
}

ast_node eat_elaborated_type_specifier_class_sem(parse_state& ps, decl_flags& flags) {
    REWIND_SCOPE(elaborated_type_specifier_class);
    e_class_key key;
    if (!eat_class_key_2(ps, key)) {
        return ast_node::null();
    }

    // TODO:
    // class-key simple-template-id
    // class-key nested-name-specifier templateopt simple-template-id

    eat_attribute_specifier_seq(ps);

    // TODO:
    // class-key attribute-specifier-seqopt nested-name-specifier identifier

    ast_node ident = eat_identifier_2(ps);
    if (!ident) {
        REWIND_ON_EXIT(elaborated_type_specifier_class);
        return ast_node::null();
    }

    auto scope = ps.get_current_scope();
    int lookup_flags
        = LOOKUP_FLAG_CLASS
        | LOOKUP_FLAG_ENUM;
    auto sym = scope->lookup(ident.as<ast::identifier>()->tok.str.c_str(), lookup_flags);

    if (peek(ps, ";")) {
        sym = get_symbol_for_redeclaration(ps, ident.as<ast::identifier>()->tok.str.c_str(), lookup_flags);
        //sym = scope->get_symbol_for_redeclaration(ident.as<ast::identifier>()->tok.str.c_str(), lookup_flags);
        //sym = scope->get_symbol(ident.as<ast::identifier>()->tok.str.c_str(), lookup_flags);
        if (sym && !sym->is<symbol_class>()) {
            throw parse_exception("class already declared as something else", ps.get_latest_token());
        }
        if (!sym) {
            sym = ps.create_symbol<symbol_class>(scope, ident.as<ast::identifier>()->tok.str.c_str());
        }
    } else if(!sym) {
        auto ns_scope = scope->navigate_to_namespace_scope();
        sym = ns_scope->get_symbol(ident.as<ast::identifier>()->tok.str.c_str(), lookup_flags);
        if (sym && !sym->is<symbol_class>()) {
            throw parse_exception("class already declared as something else", ps.get_latest_token());
        }
        if (!sym) {
            sym = ps.create_symbol<symbol_class>(ns_scope, ident.as<ast::identifier>()->tok.str.c_str());
        }
    }

    if (sym && !sym->is<symbol_class>()) {
        throw parse_exception("class already declared as something else", ps.get_latest_token());
    }

    ident.as<ast::identifier>()->sym = sym;
    /*
    if (peek(ps, ";") || !sym) {        
        if (flags.has(e_friend)) {
            if (!sym) {
                throw parse_exception("undeclared identifier for a friend declaration", ps.get_latest_token());
            }
        } else {
            sym = scope->get_symbol(ident.as<ast::identifier>()->tok.str.c_str(), lookup_flags);
            if (!sym) {
                sym = ps.create_symbol<symbol_class>(scope, ident.as<ast::identifier>()->tok.str.c_str());
            }
        }
        ident.as<ast::identifier>()->sym = sym;
    } else {
        ident.as<ast::identifier>()->sym = sym;
    }*/

    return ast_node::make<ast::elaborated_type_specifier>(key, std::move(ident), sym);
}

ast_node eat_elaborated_type_specifier_enum_sem(parse_state& ps) {
    REWIND_SCOPE(elaborated_type_specifier_enum);
    if (!eat_keyword(ps, kw_enum)) {
        return ast_node::null();
    }

    // TODO:
    // enum nested-name-specifier identifier

    ast_node ident = eat_identifier_2(ps);
    if (!ident) {
        REWIND_ON_EXIT(elaborated_type_specifier_enum);
        return ast_node::null();
    }

    auto scope = ps.get_current_scope();
    int lookup_flags
        = LOOKUP_FLAG_CLASS
        | LOOKUP_FLAG_ENUM;
    auto sym = scope->lookup(ident.as<ast::identifier>()->tok.str.c_str(), lookup_flags);

    if (sym && !sym->is<symbol_enum>()) {
        throw parse_exception("enum already declared as something else", ps.get_latest_token());
    }

    if (peek(ps, ";")) {
        throw parse_exception("enum E; is ill-formed", ps.get_latest_token());
    }

    if (!sym) {
        throw parse_exception("elaborated_type_specifier_enum: undeclared identifier", ps.get_latest_token());
    }

    ident.as<ast::identifier>()->sym = sym;

    return ast_node::make<ast::elaborated_type_specifier>(ck_enum, std::move(ident), sym);
}

ast_node eat_elaborated_type_specifier_sem(parse_state& ps, decl_flags& flags) {
    ast_node n = ast_node::null();

    n = eat_elaborated_type_specifier_class_sem(ps, flags);
    if(n) return n;

    n = eat_elaborated_type_specifier_enum_sem(ps);
    if(n) return n;

    return ast_node::null();
}

ast_node eat_elaborated_type_specifier_2(parse_state& ps, decl_flags& flags) {
    if (ps.is_semantic()) {
        return eat_elaborated_type_specifier_sem(ps, flags);
    } else {
        return eat_elaborated_type_specifier_syn(ps);
    }
}

ast_node eat_trailing_type_specifier_2(parse_state& ps, decl_flags& flags) {
    ast_node n = ast_node::null();
    
    n = eat_simple_type_specifier_2(ps, flags);
    if(n) return n;
    
    if(flags.is_compatible(e_decl_user_defined)) {
        n = eat_elaborated_type_specifier_2(ps, flags);
        if(n) {
            flags.add(e_decl_user_defined);
            return n;
        }
    }
    
    if(flags.is_compatible(e_decl_user_defined)) {
        n = eat_typename_specifier_2(ps);
        if(n) {
            flags.add(e_decl_user_defined);
            return n;
        }
    }
    
    n = eat_cv_qualifier_2(ps, flags);
    if(n) return n;

    return ast_node::null();
}

bool eat_enum_key_2(parse_state& ps, e_enum_key& key) {
    if (!accept(ps, kw_enum)) {
        return false;
    }

    if (accept(ps, kw_class)) {
        key = e_enum_key::ck_enum_class;
    } else if (accept(ps, kw_struct)) {
        key = e_enum_key::ck_enum_struct;
    } else {
        key = e_enum_key::ck_enum;
    }
    return true;
}

ast_node eat_enum_base_2(parse_state& ps) {
    if (!accept(ps, ":")) {
        return ast_node::null();
    }

    ast_node tss = eat_type_specifier_seq_2(ps);
    if(!tss) throw parse_exception("Expected a type-specifier-seq", ps.peek_token());
    return tss;
}

// Unused?
ast_node eat_enum_head_2(parse_state& ps) {
    e_enum_key key;
    if (!eat_enum_key_2(ps, key)) {
        return ast_node::null();
    }

    bool has_attribs = eat_attribute_specifier_seq(ps);

    ast_node name = ast_node::null();
    ast_node nns = eat_nested_name_specifier_2(ps);
    if (nns) {
        ast_node ident = eat_identifier_2(ps);
        if(!ident) throw parse_exception("Expected an identifier", ps.peek_token());
        nns.as<ast::nested_name_specifier>()->push_next(std::move(ident));
        name = std::move(nns);
    } else {
        name = eat_identifier_2(ps);
    }

    ast_node base = eat_enum_base_2(ps);

    return ast_node::make<ast::enum_head>(key, std::move(name), std::move(base));
}

ast_node eat_enumerator_definition_2(parse_state& ps) {
    ast_node ident = eat_identifier_2(ps);
    if(!ident) return ast_node::null();

    if (!accept(ps, "=")) {
        declare_enumerator(ps, ident.as<ast::identifier>(), nullptr);
        return ast_node::make<ast::enumerator_definition>(std::move(ident), ast_node::null());
    }

    ast_node initializer = eat_constant_expression_2(ps);
    declare_enumerator(ps, ident.as<ast::identifier>(), initializer.as<expression>());
    return ast_node::make<ast::enumerator_definition>(std::move(ident), std::move(initializer));
}

ast_node eat_enumerator_list_2(parse_state& ps) {
    ast_node n = eat_enumerator_definition_2(ps);
    if(!n) return ast_node::null();

    ast_node list = ast_node::make<ast::enumerator_list>();
    list.as<ast::enumerator_list>()->push_back(std::move(n));

    while (accept(ps, ",")) {
        n = eat_enumerator_definition_2(ps);
        if(!n) break;
        list.as<ast::enumerator_list>()->push_back(std::move(n));
    }
    return list;
}

ast_node eat_enum_specifier_2(parse_state& ps) {
    REWIND_SCOPE(enum_specifier);
    /*
    ast_node head = eat_enum_head_2(ps);
    if(!head) return ast_node::null();
    */

    // enum-head
    symbol_ref sym = nullptr;
    ast_node head = ast_node::null();
    {
        e_enum_key key;
        if (!eat_enum_key_2(ps, key)) {
            return ast_node::null();
        }

        bool has_attribs = eat_attribute_specifier_seq(ps);

        // TODO: NESTED NAME SPECIFIER
        /*
        ast_node name = ast_node::null();
        ast_node nns = eat_nested_name_specifier_2(ps);
        if (nns) {
            ast_node ident = eat_identifier_2(ps);
            if(!ident) throw parse_exception("Expected an identifier", ps.peek_token());
            nns.as<ast::nested_name_specifier>()->push_next(std::move(ident));
            name = std::move(nns);
        } else {
            name = eat_identifier_2(ps);
        }*/

        ast_node name = eat_identifier_2(ps);
        if(name) {
            symbol_table_ref cur_scope = ps.get_current_scope();
            int lookup_flags
                = LOOKUP_FLAG_NAMESPACE
                | LOOKUP_FLAG_CLASS
                | LOOKUP_FLAG_TYPEDEF
                | LOOKUP_FLAG_ENUM;
            sym = cur_scope->get_symbol(name.as<ast::identifier>()->tok.str, lookup_flags);
            name.as<ast::identifier>()->sym = sym;
        }

        if (sym && !sym->is<symbol_enum>()) {
            throw parse_exception("symbol already declared as different kind", ps.get_latest_token());
        }

        ast_node base = eat_enum_base_2(ps);

        head = ast_node::make<ast::enum_head>(key, std::move(name), std::move(base));
    }

    // { enumerator-list(opt) }
    // { enumerator-list , }

    if (!accept(ps, "{")) {
        REWIND_ON_EXIT(enum_specifier);
        return ast_node::null();
    }

    // C++14 standard states that the point of declaration of an enum
    // is immediately after its identifier,
    // but no compiler I know followed that rule,
    // so it was not visible at enum-base.
    // C++23 (or earlier, idk or care) fixed that,
    // stating that enum's locus is after enum-head.
    // We can add the symbol here,
    // after checking for {, so we don't have to
    // cancel the symbol in case there's no {

    if (sym && sym->is_defined()) {
        throw parse_exception("enum already defined", /* TODO: report the actual enum-name */ps.get_latest_token());
    }

    symbol_table_ref scope = ps.get_current_scope();
    if(!sym) {
        ast::identifier* ident = head.as<ast::enum_head>()->name.as<ast::identifier>();
        std::string name;
        if (ident) {
            name = ident->tok.str;
        }

        sym = ps.create_symbol<symbol_enum>(scope, name.c_str());
    }

    symbol_table_ref enum_scope = nullptr;
    enum_scope = ps.enter_scope(scope_enum);
    enum_scope->set_owner(sym);
    sym->nested_symbol_table = enum_scope;

    sym->set_defined(true);

    ast_node body = eat_enumerator_list_2(ps);

    expect(ps, "}");
    ps.exit_scope();

    return ast_node::make<ast::enum_specifier>(std::move(head), std::move(body), sym);
}

ast_node eat_type_specifier_2(parse_state& ps, decl_flags& flags) {
    ast_node n = ast_node::null();

    if(flags.is_compatible(e_decl_user_defined)) {
        n = eat_class_specifier_2(ps);
        if(n) {
            flags.add(e_decl_user_defined);
            return n;
        }
    }
    if(flags.is_compatible(e_decl_user_defined)) {
        n = eat_enum_specifier_2(ps);
        if(n) {
            flags.add(e_decl_user_defined);
            return n;
        }
    }
    n = eat_trailing_type_specifier_2(ps, flags);
    if(n) return n;
    return ast_node::null();
}

ast_node eat_type_specifier_seq_2(parse_state& ps) {
    ast_node n = ast_node::null();
    decl_flags flags;

    n = eat_type_specifier_2(ps, flags);
    if(!n) return ast_node::null();

    ast_node seq = ast_node::make<ast::type_specifier_seq>();
    seq.as<ast::type_specifier_seq>()->push_back(std::move(n));

    while (n = eat_type_specifier_2(ps, flags)) {
        seq.as<ast::type_specifier_seq>()->push_back(std::move(n));
    }

    seq.as<ast::type_specifier_seq>()->flags = flags;

    return seq;
}

ast_node eat_storage_class_specifier_2(parse_state& ps, decl_flags& flags) {
    if (accept(ps, kw_register)) {
        flags.add(e_storage_register);
        return ast_node::make<ast::storage_class_specifier>(kw_register);
    }
    if (accept(ps, kw_static)) {
        flags.add(e_storage_static);
        return ast_node::make<ast::storage_class_specifier>(kw_static);
    }
    if (accept(ps, kw_thread_local)) {
        flags.add(e_storage_thread_local);
        return ast_node::make<ast::storage_class_specifier>(kw_thread_local);
    }
    if (accept(ps, kw_extern)) {
        flags.add(e_storage_extern);
        return ast_node::make<ast::storage_class_specifier>(kw_extern);
    }
    if (accept(ps, kw_mutable)) {
        flags.add(e_storage_mutable);
        return ast_node::make<ast::storage_class_specifier>(kw_mutable);
    }
    return ast_node::null();
}

ast_node eat_function_specifier_2(parse_state& ps, decl_flags& flags) {    
    if (accept(ps, kw_inline)) {
        flags.add(e_fn_inline);
        return ast_node::make<ast::function_specifier>(kw_inline);
    }
    if (accept(ps, kw_virtual)) {
        flags.add(e_fn_virtual);
        return ast_node::make<ast::function_specifier>(kw_virtual);
    }
    if (accept(ps, kw_explicit)) {
        flags.add(e_fn_explicit);
        return ast_node::make<ast::function_specifier>(kw_explicit);
    }
    return ast_node::null();
}

ast_node eat_decl_specifier_2(parse_state& ps, decl_flags& flags) {
    ast_node n = ast_node::null();
    
    n = eat_storage_class_specifier_2(ps, flags);
    if(n) return n;

    n = eat_type_specifier_2(ps, flags);
    if(n) return n;

    n = eat_function_specifier_2(ps, flags);
    if(n) return n;

    if (accept(ps, kw_friend)) {
        flags.add(e_friend);
        return ast_node::make<ast::decl_specifier>(kw_friend);
    }
    if (accept(ps, kw_typedef)) {
        flags.add(e_typedef);
        return ast_node::make<ast::decl_specifier>(kw_typedef);
    }
    if (accept(ps, kw_constexpr)) {
        flags.add(e_constexpr);
        return ast_node::make<ast::decl_specifier>(kw_constexpr);
    }
    return ast_node::null();
}

ast_node eat_decl_specifier_seq_2(parse_state& ps) {
    decl_flags flags;
    ast_node n = eat_decl_specifier_2(ps, flags);
    if(!n) return ast_node::null();

    ast_node seq = ast_node::make<ast::decl_specifier_seq>();
    seq.as<ast::decl_specifier_seq>()->push_back(std::move(n));

    while (n = eat_decl_specifier_2(ps, flags)) {
        seq.as<ast::decl_specifier_seq>()->push_back(std::move(n));
    }

    seq.as<ast::decl_specifier_seq>()->flags = flags;
    return seq;
}

ast_node eat_simple_type_specifier_2(parse_state& ps, decl_flags& flags) {
    if(flags.is_compatible(e_decl_user_defined)) {
        ast_node nns = eat_nested_name_specifier_2(ps);
        symbol_table_ref qualified_scope = nullptr;
        if (nns) {
            qualified_scope = nns.as<ast::nested_name_specifier>()->get_qualified_scope(ps);
        }

        if (nns) {
            if (accept(ps, kw_template)) {
                // nested-name-specifier template simple-template-id
                ast_node stid = eat_simple_template_id_2(ps, qualified_scope);
                if (!stid) {
                    throw parse_exception("Expected a simple_template_id", ps.peek_token());
                }
                nns.as<ast::nested_name_specifier>()->push_next(std::move(stid));
                flags.add(e_decl_user_defined);
                return nns;
            }
        }

        // nested-name-specifieropt type-name
        ast_node type_name = ast_node::null();
        if (type_name = eat_type_name_sem(ps, qualified_scope)) {
            flags.add(e_decl_user_defined);
            if (nns) {
                nns.as<ast::nested_name_specifier>()->push_next(std::move(type_name));
                return nns;
            }
            return type_name;
        }

        if (nns) {
            throw parse_exception("Expected a type-name", ps.peek_token());
        }
    }

    if (flags.is_compatible(e_decl_char) && accept(ps, kw_char)) {
        flags.add(e_decl_char);
        return ast_node::make<ast::simple_type_specifier>(kw_char);
    }
    if (flags.is_compatible(e_decl_char16_t) && accept(ps, kw_char16_t)) {
        flags.add(e_decl_char16_t);
        return ast_node::make<ast::simple_type_specifier>(kw_char16_t);
    }
    if (flags.is_compatible(e_decl_char32_t) && accept(ps, kw_char32_t)) {
        flags.add(e_decl_char32_t);
        return ast_node::make<ast::simple_type_specifier>(kw_char32_t);
    }
    if (flags.is_compatible(e_decl_wchar_t) && accept(ps, kw_wchar_t)) {
        flags.add(e_decl_wchar_t);
        return ast_node::make<ast::simple_type_specifier>(kw_wchar_t);
    }
    if (flags.is_compatible(e_decl_bool) && accept(ps, kw_bool)) {
        flags.add(e_decl_bool);
        return ast_node::make<ast::simple_type_specifier>(kw_bool);
    }
    if (flags.is_compatible(e_decl_short) && accept(ps, kw_short)) {
        flags.add(e_decl_short);
        return ast_node::make<ast::simple_type_specifier>(kw_short);
    }
    if (flags.is_compatible(e_decl_int) && accept(ps, kw_int)) {
        flags.add(e_decl_int);
        return ast_node::make<ast::simple_type_specifier>(kw_int);
    }
    if (flags.is_compatible(e_decl_long) && accept(ps, kw_long)) {
        flags.add(e_decl_long);
        return ast_node::make<ast::simple_type_specifier>(kw_long);
    }
    if (flags.is_compatible(e_decl_signed) && accept(ps, kw_signed)) {
        flags.add(e_decl_signed);
        return ast_node::make<ast::simple_type_specifier>(kw_signed);
    }
    if (flags.is_compatible(e_decl_unsigned) && accept(ps, kw_unsigned)) {
        flags.add(e_decl_unsigned);
        return ast_node::make<ast::simple_type_specifier>(kw_unsigned);
    }
    if (flags.is_compatible(e_decl_float) && accept(ps, kw_float)) {
        flags.add(e_decl_float);
        return ast_node::make<ast::simple_type_specifier>(kw_float);
    }
    if (flags.is_compatible(e_decl_double) && accept(ps, kw_double)) {
        flags.add(e_decl_double);
        return ast_node::make<ast::simple_type_specifier>(kw_double);
    }
    if (flags.is_compatible(e_decl_void) && accept(ps, kw_void)) {
        flags.add(e_decl_void);
        return ast_node::make<ast::simple_type_specifier>(kw_void);
    }
    if (flags.is_compatible(e_decl_auto) && accept(ps, kw_auto)) {
        flags.add(e_decl_auto);
        return ast_node::make<ast::simple_type_specifier>(kw_auto);
    }

    if(flags.is_compatible(e_decl_decltype)) {
        ast_node n = eat_decltype_specifier_2(ps);
        if (n) {
            // Not technically correct, but good enough
            flags.add(e_decl_decltype);
            return n;
        }
    }
    return ast_node::null();
}

ast_node eat_type_name_syn(parse_state& ps) {
    ast_node n = eat_identifier_2(ps);
    if (!n) {
        return ast_node::null();
    }

    token tok;
    if (eat_token(ps, "<", &tok)) {
        ast_node nargs = eat_template_argument_list_2(ps);

        expect_closing_angle(ps);

        n = ast_node::make<ast::simple_template_id>(std::move(n), std::move(nargs));
    }

    return n;
}

ast_node eat_type_name_sem(parse_state& ps, symbol_table_ref qualified_scope) {
    REWIND_SCOPE(type_name_sem);
    token tok;
    if (!eat_token(ps, tt_identifier, &tok)) {
        return ast_node::null();
    }

    /*
        int flags
            = LOOKUP_FLAG_CLASS
            | LOOKUP_FLAG_TYPEDEF
            | LOOKUP_FLAG_TEMPLATE_PARAMETER;
        symbol_ref sym = nullptr;

        if (qualified_scope) {
            sym = qualified_scope->get_symbol(ident.as<ast::identifier>()->tok.str, flags);
        } else {
            sym = ps.get_current_scope()->lookup(ident.as<ast::identifier>()->tok.str, flags);
        }

        if (sym) {
            if (sym->as<symbol_typedef>()) {
                throw parse_exception("TODO: typedef for a class-name not yet supported", ps.get_latest_token());
            }
            if (symbol_template_parameter* param = sym->as<symbol_template_parameter>()) {
                if (param->kind != e_template_parameter_type) {
                    throw parse_exception("template-parameter expected to be a type parameter", ps.get_latest_token());
                }
            }

            return ast_node::make<ast::class_name>(sym);
        }
    */

    int flags
        = LOOKUP_FLAG_CLASS
        | LOOKUP_FLAG_ENUM
        | LOOKUP_FLAG_TYPEDEF
        | LOOKUP_FLAG_TEMPLATE
        | LOOKUP_FLAG_TEMPLATE_PARAMETER;
    symbol_ref sym = nullptr;

    if (qualified_scope) {
        sym = qualified_scope->get_symbol(tok.str, flags);
    } else {
        sym = ps.get_current_scope()->lookup(tok.str, flags);
    }

    if (!sym) {
        REWIND_ON_EXIT(type_name_sem);
        return ast_node::null();
    }

    //ast_node n = ast_node::make<ast::identifier>(tok, sym);

    if (sym->is<symbol_class>()) {
        return ast_node::make<ast::class_name>(sym);
    }

    if (sym->is<symbol_enum>()) {
        return ast_node::make<ast::enum_name>(sym);
    }

    if (sym->is<symbol_typedef>()) {
        return ast_node::make<ast::typedef_name>(sym);
    }

    if (auto param = sym->as<symbol_template_parameter>()) {
        if (param->kind == e_template_parameter_non_type) {
            REWIND_ON_EXIT(type_name_sem);
            return ast_node::null();
        }
        if (param->kind != e_template_parameter_type) {
            throw parse_exception("TODO: template-parameter expected to be a type or non_type", ps.get_latest_token());
        }
        return ast_node::make<ast::dependent_type_name>(sym);
    }

    if(sym->is<symbol_template>()) {
        ast_node n = ast_node::make<ast::identifier>(tok, sym);

        if (!accept(ps, "<")) {
            throw parse_exception("Missing template argument list", ps.get_latest_token());
        }
            
        ast_node args = eat_template_argument_list_2(ps);

        expect_closing_angle(ps);

        n = ast_node::make<ast::simple_template_id>(std::move(n), std::move(args));
        return n;
    }
    
    throw parse_exception("eat_type_name_sem() unreachable code", ps.get_latest_token());
}

ast_node eat_type_name_2(parse_state& ps) {
    if (ps.is_semantic()) {
        return eat_type_name_sem(ps);
    } else {
        return eat_type_name_syn(ps);
    }
}

ast_node eat_namespace_name_sem(parse_state& ps, symbol_table_ref qualified_scope) {
    REWIND_SCOPE(namespace_name);
    ast_node ident = eat_identifier_2(ps);
    if (!ident) {
        return ast_node::null();
    }

    symbol_ref sym = nullptr;
    if (qualified_scope) {
        sym = qualified_scope->get_symbol(ident.as<ast::identifier>()->tok.str, LOOKUP_FLAG_NAMESPACE);
    } else {
        sym = ps.get_current_scope()->lookup(ident.as<ast::identifier>()->tok.str, LOOKUP_FLAG_NAMESPACE);
    }
    if (!sym) {
        REWIND_ON_EXIT(namespace_name);
        return ast_node::null();
    }

    return ast_node::make<ast::namespace_name>(sym);
}

ast_node eat_decltype_specifier_2(parse_state& ps) {
    if (!accept(ps, kw_decltype)) {
        return ast_node::null();
    }

    expect(ps, "(");
    if (accept(ps, kw_auto)) {
        return ast_node::make<ast::decltype_specifier>();
    }
    ast_node expr = eat_expression_2(ps);
    if (!expr) {
        throw parse_exception("Expected an expression", ps.peek_token());
    }
    expect(ps, ")");
    return ast_node::make<ast::decltype_specifier>(std::move(expr));
}

ast_node eat_simple_declaration_2(parse_state& ps) {
    REWIND_SCOPE(simple_declaration);
    
    bool has_attribs = eat_attribute_specifier_seq(ps);
    
    ast_node decl_spec_seq = eat_decl_specifier_seq_2(ps);
    auto decl_specifier_seq = decl_spec_seq.as<ast::decl_specifier_seq>();

    ast_node init_decl_list = eat_init_declarator_list_2(ps, decl_specifier_seq);
    if (has_attribs && decl_spec_seq && !init_decl_list) {
        throw parse_exception("init-declarator-list is required when attribute-specifier-seq is present", ps.peek_token());
    }

    if (!decl_spec_seq && !init_decl_list) {
        REWIND_ON_EXIT(simple_declaration);
        return ast_node::null();
    }

    // Check if this declaration declares anything at all.
    // Probably a redundant check for a parser, might remove
    bool declares_anything = true;
    if (decl_spec_seq && !init_decl_list) {
        declares_anything = false;
        if (decl_specifier_seq->flags.has(e_decl_user_defined)) {
            for (int i = 0; i < decl_specifier_seq->specifiers.size(); ++i) {
                const ast_node& spec = decl_specifier_seq->specifiers[i];
                if (auto class_spec = spec.as<ast::class_specifier>()) {
                    const ast::class_head* head = class_spec->head.as<ast::class_head>();
                    if(head && head->class_head_name) {
                        declares_anything = true;
                        break;
                    }
                }
                if (auto enum_spec = spec.as<ast::enum_specifier>()) {
                    const ast::enum_head* head = enum_spec->head.as<ast::enum_head>();
                    if(head && head->name) {
                        declares_anything = true;
                        break;
                    }
                }
                if (auto elab = spec.as<ast::elaborated_type_specifier>()) {
                    if (elab->type == e_elaborated_class) {
                        declares_anything = true;
                        break;
                    }
                }
            }
        }
    }

    auto init_declarator_list = init_decl_list.as<ast::init_declarator_list>();

    if(accept(ps, ";")) {
        if (!declares_anything) {
            throw parse_exception("simple-declaration does not declare anything", ps.get_latest_token());
        }

        return ast_node::make<ast::simple_declaration>(std::move(decl_spec_seq), std::move(init_decl_list));
    }

    if (!init_decl_list) {
        REWIND_ON_EXIT(simple_declaration);
        return ast_node::null();
    }

    if (init_declarator_list->list.size() == 1 &&
        init_declarator_list->list[0].as<ast::init_declarator>() &&
        init_declarator_list->list[0].as<ast::init_declarator>()->declarator.as<ast::declarator>()->is_function()
    ) {
        ast_node body = eat_function_body_2(ps);
        return ast_node::make<ast::function_definition>(std::move(decl_spec_seq), std::move(init_declarator_list->list[0]), std::move(body));
    }
    REWIND_ON_EXIT(simple_declaration);
    return ast_node::null();
}

ast_node eat_static_assert_declaration_2(parse_state& ps) {
    if (!accept(ps, kw_static_assert)) {
        return ast_node::null();
    }
    expect(ps, "(");

    ast_node const_expr = eat_constant_expression_2(ps);
    if (!const_expr) {
        throw parse_exception("static_assert requires a constant expression", ps.peek_token());
    }

    ast_node strlit = ast_node::null();
    token strlit_tok;
    if (accept(ps, ",")) {
        if (!eat_token(ps, tt_string_literal, &strlit_tok)) {
            throw parse_exception("string literal required after comma in static_assert declaration", ps.peek_token());
        }
        strlit = ast_node::make<ast::literal>(strlit_tok);
    }

    expect(ps, ")");
    expect(ps, ";");

    eval_result res = const_expr.try_evaluate();
    dbg_printf_color("static_assert: ", DBG_WHITE);
    res.dbg_print();
    dbg_printf_color(": %s", DBG_STRING_LITERAL, strlit_tok.strlit_content.c_str());
    dbg_printf_color("\n", DBG_WHITE);
    // TODO:
    if (!res.value.u64) {
        token t = ps.get_latest_token();
        throw parse_exception(strlit_tok.strlit_content.c_str(), t.file->file_name.c_str(), "static_assert", t.line, t.col);
    }

    return ast_node::make<ast::static_assert_>(std::move(const_expr), std::move(strlit));
}

ast_node eat_alias_declaration_2(parse_state& ps) {
    REWIND_SCOPE(alias_declaration);
    if (!accept(ps, kw_using)) {
        return ast_node::null();
    }

    ast_node ident = eat_identifier_2(ps);
    if (!ident) {
        REWIND_ON_EXIT(alias_declaration);
        return ast_node::null();
    }

    bool has_attribs = eat_attribute_specifier_seq(ps);
    if (!accept(ps, "=")) {
        REWIND_ON_EXIT(alias_declaration);
        return ast_node::null();
    }

    ast_node tid = eat_type_id_2(ps);
    if(!tid) throw parse_exception("Expected a type-id", ps.peek_token());

    expect(ps, ";");

    declare_alias(ps, ident.as<ast::identifier>(), tid.as<ast::type_id>());

    return ast_node::make<ast::alias_declaration>(std::move(ident), std::move(tid));
}

ast_node eat_opaque_enum_declaration_2(parse_state& ps) {
    REWIND_SCOPE(opaque_enum_declaration);
    e_enum_key key;
    if (!eat_enum_key_2(ps, key)) {
        return ast_node::null();
    }

    bool has_attribs = eat_attribute_specifier_seq(ps);

    ast_node name = eat_identifier_2(ps);
    if (!name) {
        REWIND_ON_EXIT(opaque_enum_declaration);
        return ast_node::null();
    }

    ast_node base = eat_enum_base_2(ps);

    if (!accept(ps, ";")) {
        REWIND_ON_EXIT(opaque_enum_declaration);
        return ast_node::null();
    }

    auto scope = ps.get_current_scope();
    int lookup_flags
        = LOOKUP_FLAG_ENUM
        | LOOKUP_FLAG_CLASS;
    auto sym = scope->get_symbol(name.as<ast::identifier>()->tok.str.c_str(), lookup_flags);

    if (sym && !sym->is<symbol_enum>()) {
        throw parse_exception("enum name already in use by something else", ps.get_latest_token());
    }

    if (!sym) {
        sym = ps.create_symbol<symbol_enum>(0, name.as<ast::identifier>()->tok.str.c_str());
        name.as<ast::identifier>()->sym = sym;
    } else {
        // Do nothing
        // could check for base match and scoped-unscoped, but won't
    }

    return ast_node::make<ast::opaque_enum_decl>(key, std::move(name), std::move(base));
}

ast_node eat_block_declaration_2(parse_state& ps) {
    ast_node n = ast_node::null();

    n = eat_opaque_enum_declaration_2(ps);
    if(n) return n;

    n = eat_simple_declaration_2(ps);
    if(n) return n;

    // TODO:
    // asm-definition
    // namespace-alias-definition
    // using-declaration
    // using-directive
    
    n = eat_static_assert_declaration_2(ps);
    if(n) return n;

    n = eat_alias_declaration_2(ps);
    if(n) return n;    

    return ast_node::null();
}

ast_node eat_declaration_2(parse_state& ps) {
    ast_node n = ast_node::null();

    n = eat_block_declaration_2(ps);
    if(n) return n;

    // function-definition
    // handled in eat_simple_declaration()
    
    n = eat_template_declaration_2(ps);
    if(n) return n;

    // TODO:
    // explicit-instantiation
    // explicit-specialization
    // linkage-specification
    
    n = eat_namespace_definition_2(ps);
    if(n) return n;

    // empty-declaration
    if (accept(ps, ";")) {
        return ast_node::make<ast::dummy>();
    }

    {
        REWIND_SCOPE(attr_decl);
        ast_node attr = eat_attribute_specifier_seq_2(ps);
        if (attr && accept(ps, ";")) {
            return attr;
        }/*
        bool has_attribs = eat_attribute_specifier_seq(ps);
        if(has_attribs && accept(ps, ";")) {
            // TODO:
            return ast_node::make<ast::dummy>();
        }*/
        REWIND_ON_EXIT(attr_decl);
    }

    return ast_node::null();
}

bool eat_declaration_seq_limited_block_2(parse_state& ps, ast::declaration_seq* seq) {
    REWIND_SCOPE(declaration_seq_limited_block);
    {
        attribute_specifier attr_spec;
        if (!eat_attribute_specifier_seq(ps, attr_spec)) {
            return false;
        }
        if (!attr_spec.find_attrib("cppi_begin")) {
            REWIND_ON_EXIT(declaration_seq_limited_block);
            return false;
        }
        accept(ps, ";");
        if (attr_spec.find_attrib("no_reflect")) {
            // TODO:
            //ps.no_reflect = true;
        }
        ps.set_limited(false);
    }

    while (true) {
        ast_node n = eat_declaration_2(ps);
        if (n) {
            if (n.is<ast::attributes>()) {
                if(n.as<ast::attributes>()->attribs.find_attrib("cppi_end")) {
                    ps.set_limited(true);
                    break;
                }
            }
            seq->push_back(std::move(n));
            continue;
        }
        
        throw parse_exception("Unknown syntax or end of file encountered before [[cppi_end]]", ps.peek_token());
    }

    return true;
}

ast_node eat_declaration_seq_limited_2(parse_state& ps) {
    ast_node decl_seq = ast_node::make<ast::declaration_seq>();
    auto seq = decl_seq.as<ast::declaration_seq>();
    while(true) {
        ast_node n = ast_node::null();
        n = eat_namespace_definition_2(ps);
        if(n) {
            seq->push_back(std::move(n));
            continue;
        }

        // cppi_begin -> cppi_end
        if (eat_declaration_seq_limited_block_2(ps, seq)) {
            continue;
        }

        attribute_specifier attr_spec;
        if (eat_attribute_specifier_seq(ps, attr_spec)) {
            if (attr_spec.find_attrib("no_reflect")) {
                // TODO:
                //ps.no_reflect = true;
            }

            if (attr_spec.find_attrib("cppi_class")) {
                expect(ps, ";");
                ast_node n = eat_class_specifier_2(ps);
                if(!n) throw parse_exception("cppi_class: expected a class-specifier", ps.peek_token());
                seq->push_back(std::move(n));
                continue;
            } else if (attr_spec.find_attrib("cppi_enum")) {
                expect(ps, ";");
                ast_node n = eat_enum_specifier_2(ps);
                if(!n) throw parse_exception("cppi_enum: expected an enum-specifier", ps.peek_token());
                seq->push_back(std::move(n));
                continue;
            } else if (attr_spec.find_attrib("cppi_decl")) {
                accept(ps, ";");
                ast_node n = eat_declaration_2(ps);
                if(!n) throw parse_exception("cppi_decl: expected a declaration", ps.peek_token());
                seq->push_back(std::move(n));
                continue;
            } else if (attr_spec.find_attrib("cppi_tpl")) {
                expect(ps, ";");
                ast_node n = eat_template_declaration_2(ps);
                if(!n) throw parse_exception("cppi_tpl: expected a template-declaration", ps.peek_token());
                seq->push_back(std::move(n));
                continue;
            }
        }

        if (eat_balanced_token(ps)) {
            continue;
        }
        break;
    }
    if (seq->seq.empty()) {
        return ast_node::null();
    }
    return decl_seq;
    /*
    attribute_specifier attr_spec;
    if (eat_namespace_definition_2(ps)) {
        continue;
    } else if (eat_attribute_specifier_seq(ps, attr_spec)) {
        if (attr_spec.find_attrib("cppi_begin")) {
            if (attr_spec.find_attrib("no_reflect")) {
                ps.no_reflect = true;
            }
            expect(ps, ";");
            eat_cppi_block(ps, attr_spec);
            continue;
        } else if (attr_spec.find_attrib("cppi_class")) {
            expect(ps, ";");
            if (!eat_class_specifier_limited(ps, attr_spec)) {
                throw parse_exception("cppi_class attribute must be followed by a class definition", ps.latest_token);
            }
            continue;
        } else if (attr_spec.find_attrib("cppi_class_tpl")) {
            expect(ps, ";");
            if (!eat_template_class_specifier_limited(ps, attr_spec)) {
                throw parse_exception("cppi_class_tpl attribute must be followed by a template class definition", ps.latest_token);
            }
            continue;
        } else if (attr_spec.find_attrib("cppi_enum")) {
            expect(ps, ";");
            if (!eat_enum_specifier_limited(ps)) {
                throw parse_exception("cppi_enum attribute must be followed by an enumeration definition", ps.latest_token);
            }
            continue;
        }

        continue;
    }

    if (eat_balanced_token(ps)) {
        continue;
    }*/
}

ast_node eat_declaration_seq_2(parse_state& ps) {
    int check = ps.get_token_cache_cur();
    ast_node n = eat_declaration_2(ps);
    if (!n) {
        if (check != ps.get_token_cache_cur()) {
            throw parse_exception("Did not rewind properly at global or namespace level", ps.get_latest_token());
        }
        return ast_node::null();
    }

    ast_node seq = ast_node::make<ast::declaration_seq>();
    seq.as<ast::declaration_seq>()->push_back(std::move(n));

    while (true) {
        int check = ps.get_token_cache_cur();

        n = eat_declaration_2(ps);
        if (!n) {
            if (check != ps.get_token_cache_cur()) {
                throw parse_exception("Did not rewind properly at global or namespace level", ps.get_latest_token());
            }
            break;
        }
        seq.as<ast::declaration_seq>()->push_back(std::move(n));
    }
    return seq;
}

ast_node eat_namespace_definition_2(parse_state& ps) {
    REWIND_SCOPE(namespace_definition);
    bool is_inline = false;
    if (accept(ps, kw_inline)) {
        is_inline = true;
    }

    if (!accept(ps, kw_namespace)) {
        REWIND_ON_EXIT(namespace_definition);
        return ast_node::null();
    }

    bool is_anon = false;
    token tok;
    ast_node ident;
    if (eat_token(ps, tt_identifier, &tok)) {
        ident = ast_node::make<ast::identifier>(tok);
    } else {
        is_anon = true;
    }

    static int anon_counter = 0; // TODO: make it thread safe pls
    std::string namespace_name = "__anon__" + std::to_string(anon_counter++);
    if(!is_anon) {
        namespace_name = tok.str;
    }

    // GET or CREATE the namespace symbol
    // put it into current scope
    // SET DEFINED
    symbol_ref sym = nullptr;
    if(!is_anon) {
        sym = ps.get_current_scope()->get_symbol(namespace_name, LOOKUP_FLAG_NAMESPACE);
    }

    if (sym && !sym->as<symbol_namespace>()->is_inline && is_inline) {
        throw parse_exception("namespace must be specified inline at its initial definition", ps.get_latest_token());
    }

    if (!sym) {
        sym = ps.get_current_scope()->create_symbol<symbol_namespace>(namespace_name.c_str(), ps.get_latest_token().file, false);
        sym->set_defined(true);
        sym->as<symbol_namespace>()->is_anon = is_anon;
        sym->as<symbol_namespace>()->is_inline = is_inline;
    }

    expect(ps, "{");
    // CREATE new scope or ENTER existing
    // ATTACH scope to symbol if no existing
    if (sym->nested_symbol_table) {
        ps.enter_scope(sym->nested_symbol_table);
    } else {
        sym->nested_symbol_table = ps.enter_scope(scope_namespace);
        sym->nested_symbol_table->set_owner(sym);
    }

    ast_node body = ast_node::null();
    if (ps.is_limited()) {
        body = eat_declaration_seq_limited_2(ps);
    } else {
        body = eat_declaration_seq_2(ps);
    }

    expect(ps, "}");
    // EXIT scope
    ps.exit_scope();

    return ast_node::make<ast::namespace_definition>(std::move(ident), std::move(body), is_inline);
}

