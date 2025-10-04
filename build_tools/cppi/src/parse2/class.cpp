#include "parse.hpp"


bool eat_class_key_2(parse_state& ps, e_class_key& out_key) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_keyword) {
        ps.rewind_();
        return false;
    }
    switch (tok.kw_type) {
    case kw_class: out_key = ck_class; ps.pop_rewind_point(); return true;
    case kw_struct: out_key = ck_struct; ps.pop_rewind_point(); return true;
    case kw_union: out_key = ck_union; ps.pop_rewind_point(); return true;
    }
    ps.rewind_();
    return false;
}
ast_node eat_class_name_2(parse_state& ps) {
    ast_node n = eat_simple_template_id_2(ps);
    if(n) return n;
    return eat_identifier_2(ps);
}

ast_node eat_class_head_name_syn(parse_state& ps) {
    REWIND_SCOPE(class_head_name);
    ast_node nns = eat_nested_name_specifier_2(ps);
    ast_node class_name = eat_class_name_2(ps);
    if (!class_name) {
        REWIND_ON_EXIT(class_head_name);
        return ast_node::null();
    }

    if (nns) {
        nns.as<ast::nested_name_specifier>()->push_next(std::move(class_name));
        return nns;
    } else {
        return class_name;
    }
}

ast_node eat_class_head_name_sem(parse_state& ps) {    
    REWIND_SCOPE(class_head_name);
    // TODO: nested-name-specifier, simple-template-id
    /*
    ast_node nns = eat_nested_name_specifier_2(ps);
    ast_node class_name = eat_class_name_2(ps);
    if (!class_name) {
        REWIND_ON_EXIT(class_head_name);
        return ast_node::null();
    }

    if (nns) {
        nns.as<ast::nested_name_specifier>()->push_next(std::move(class_name));
        return nns;
    } else {
        return class_name;
    }*/

    ast_node name = eat_identifier_2(ps);
    if (!name) {
        return ast_node::null();
    }
    
    symbol_table_ref cur_scope = ps.get_current_scope();
    int lookup_flags
        = LOOKUP_FLAG_ENUM
        | LOOKUP_FLAG_NAMESPACE
        | LOOKUP_FLAG_TYPEDEF
        | LOOKUP_FLAG_CLASS;
    symbol_ref sym = cur_scope->lookup(name.as<ast::identifier>()->tok.str, lookup_flags);
    if (sym && !sym->is<symbol_class>()) {
        throw parse_exception("name already declared as something else", ps.get_latest_token());
    }
    name.as<ast::identifier>()->sym = sym;
    return name;
}

ast_node eat_class_head_name_2(parse_state& ps) {
    if (ps.is_semantic()) {
        return eat_class_head_name_sem(ps);
    } else {
        return eat_class_head_name_syn(ps);
    }
}

bool eat_access_specifier_2(parse_state& ps, e_access_specifier& access) {
    if (accept(ps, kw_private)) {
        access = e_access_private;
        return true;
    } else if (accept(ps, kw_protected)) {
        access = e_access_protected;
        return true;
    } else if (accept(ps, kw_public)) {
        access = e_access_public;
        return true;
    }
    return false;
}

ast_node eat_class_or_decltype_2(parse_state& ps) {
    REWIND_SCOPE(class_or_decltype);
    ast_node nns = eat_nested_name_specifier_2(ps);
    if (nns) {
        ast_node class_name = eat_class_name_2(ps);
        if (!class_name) {
            REWIND_ON_EXIT(class_or_decltype);
            return ast_node::null();
        }
        nns.as<ast::nested_name_specifier>()->push_next(std::move(class_name));
        return nns;
    }


    ast_node class_name = eat_class_name_2(ps);
    if (class_name) {
        if (nns) {
            nns.as<ast::nested_name_specifier>()->push_next(std::move(class_name));
            return nns;
        } else {
            return class_name;
        }
    }

    if (!nns) {
        ast_node decltype_spec = eat_decltype_specifier_2(ps);
        if (decltype_spec) {
            return decltype_spec;
        }
    } else {
        throw parse_exception("Expected a class-name", ps.peek_token());
    }

    return ast_node::null();
}

ast_node eat_base_type_specifier_2(parse_state& ps) {
    return eat_class_or_decltype_2(ps);
}

ast_node eat_base_specifier_2(parse_state& ps) {
    REWIND_SCOPE(base_specifier_);
    eat_attribute_specifier_seq(ps);

    bool is_virtual = false;
    e_access_specifier access = e_access_unspecified;
    if (accept(ps, kw_virtual)) {
        is_virtual = true;
        eat_access_specifier_2(ps, access);
    } else if (eat_access_specifier_2(ps, access)) {
        if (accept(ps, kw_virtual)) {
            is_virtual = true;
        }
    }

    ast_node bts = eat_base_type_specifier_2(ps);
    if (!bts) {
        REWIND_ON_EXIT(base_specifier_);
        return ast_node::null();
    }
    return ast_node::make<ast::base_specifier>(std::move(bts), access, is_virtual);
}

ast_node eat_base_specifier_list_2(parse_state& ps) {
    ast_node n = eat_base_specifier_2(ps);
    if(!n) return ast_node::null();
    if (accept(ps, "...")) {
        n.as<ast::base_specifier>()->is_pack_expansion = true;
    }

    ast_node list = ast_node::make<ast::base_specifier_list>();
    list.as<ast::base_specifier_list>()->push_back(std::move(n));

    while (accept(ps, ",")) {
        n = eat_base_specifier_2(ps);
        if(!n) throw parse_exception("Expected a base-specifier", ps.peek_token());
        if (accept(ps, "...")) {
            n.as<ast::base_specifier>()->is_pack_expansion = true;
        }
        list.as<ast::base_specifier_list>()->push_back(std::move(n));
    }

    return list;
}

ast_node eat_base_clause_2(parse_state& ps) {
    if (!accept(ps, ":")) {
        return ast_node::null();
    }

    ast_node list = eat_base_specifier_list_2(ps);
    if(!list) throw parse_exception("Expected a base-specifier-list", ps.peek_token());
    return list;
}

ast_node eat_class_head_2(parse_state& ps) {
    e_class_key key;
    if (!eat_class_key_2(ps, key)) {
        return ast_node::null();
    }

    eat_attribute_specifier_seq(ps);
    ast_node class_head_name = eat_class_head_name_2(ps);
    bool is_final = false;
    {
        REWIND_SCOPE(class_virt_specifier);
        if (class_head_name && accept(ps, "final")) {
            if (peek(ps, ":")) {
                is_final = true;
            } else {
                REWIND_ON_EXIT(class_virt_specifier);
            }
        }
    }

    ast_node base_clause = eat_base_clause_2(ps);

    return ast_node::make<ast::class_head>(key, std::move(class_head_name), std::move(base_clause));
}

bool eat_virt_specifier_2(parse_state& ps, e_virt& virt) {
    if (accept(ps, "override")) {
        virt = e_override;
        return true;
    }

    if (accept(ps, "final")) {
        virt = e_final;
        return true;
    }

    return false;
}

e_virt_flags eat_virt_specifier_seq_2(parse_state& ps) {
    e_virt virt = e_virt_none;
    if (!eat_virt_specifier_2(ps, virt)) {
        return 0;
    }

    e_virt_flags flags = virt;

    while (eat_virt_specifier_2(ps, virt)) {
        flags |= virt;
    }
    return flags;
}

bool eat_pure_specifier_2(parse_state& ps) {
    REWIND_SCOPE(pure_specifier);
    if (!accept(ps, "=")) {
        return false;
    }
    if (!accept(ps, "0")) {
        REWIND_ON_EXIT(pure_specifier);
        return false;
    }
    return true;
}


ast_node eat_member_declarator_2(parse_state& ps) {
    // declarator virt-specifier-seq(opt) pure-specifier(opt)
    // declarator brace-or-equal-initializer(opt)
    // identifier(opt) attribute-specifier-seq(opt) : constant-expression

    ast_node declarator = eat_declarator_2(ps);

    if (!declarator || declarator.as<ast::declarator>()->is_plain_identifier()) {
        REWIND_SCOPE(bitfield);
        bool has_attribs = eat_attribute_specifier_seq(ps);
        if (accept(ps, ":")) {
            ast_node const_expr = eat_constant_expression_2(ps);
            if(!const_expr) throw parse_exception("Expected a constant-expression", ps.peek_token());
            return ast_node::make<ast::bit_field>(std::move(declarator), std::move(const_expr));
        }
        REWIND_ON_EXIT(bitfield);
    }

    if (declarator && declarator.as<ast::declarator>()->is_function()) {
        e_virt_flags virt_flags = eat_virt_specifier_seq_2(ps);
        bool is_pure = eat_pure_specifier_2(ps);
        return ast_node::make<ast::member_function_declarator>(std::move(declarator), virt_flags, is_pure);
    }

    if (declarator) {
        ast_node initializer = eat_brace_or_equal_initializer_2(ps);
        return ast_node::make<ast::member_declarator>(std::move(declarator), std::move(initializer));
    }

    return ast_node::null();
}

ast_node eat_member_declarator_list_2(parse_state& ps) {
    ast_node n = eat_member_declarator_2(ps);
    if(!n) return ast_node::null();

    ast_node list = ast_node::make<ast::member_declarator_list>();
    list.as<ast::member_declarator_list>()->push_back(std::move(n));

    while (accept(ps, ",")) {
        n = eat_member_declarator_2(ps);
        if(!n) throw parse_exception("Expected a declarator", ps.peek_token());
        list.as<ast::member_declarator_list>()->push_back(std::move(n));
    }
    return list;
}

ast_node eat_mem_initializer_id_2(parse_state& ps) {
    // identifier already covered by eat_class_or_decltype()
    return eat_class_or_decltype_2(ps);
}

ast_node eat_mem_initializer_2(parse_state& ps) {
    REWIND_SCOPE(mem_initializer);
    ast_node n = eat_mem_initializer_id_2(ps);
    if(!n) return ast_node::null();

    if (accept(ps, "(")) {
        ast_node expr_list = eat_expression_list_2(ps);
        expect(ps, ")");
        return ast_node::make<ast::mem_initializer>(std::move(n), std::move(expr_list));
    }

    ast_node bil = eat_braced_init_list_2(ps);
    if (!bil) {
        REWIND_ON_EXIT(mem_initializer);
        return ast_node::null();
    }

    return ast_node::make<ast::mem_initializer>(std::move(n), std::move(bil));
}

ast_node eat_mem_initializer_list_2(parse_state& ps) {
    ast_node n = ast_node::null();
    n = eat_mem_initializer_2(ps);
    if(!n) return ast_node::null();
    if (accept(ps, "...")) {
        n.as<ast::mem_initializer>()->is_pack_expansion = true;
    }

    ast_node list = ast_node::make<ast::mem_initializer_list>();
    list.as<ast::mem_initializer_list>()->push_back(std::move(n));
    while (accept(ps, ",")) {
        n = eat_mem_initializer_2(ps);
        if(!n) throw parse_exception("Expected a mem-initializer", ps.peek_token());
        if (accept(ps, "...")) {
            n.as<ast::mem_initializer>()->is_pack_expansion = true;
        }
        list.as<ast::mem_initializer_list>()->push_back(std::move(n));
    }
    return std::move(list);
}

ast_node eat_ctor_initializer_2(parse_state& ps) {
    if (!accept(ps, ":")) {
        return ast_node::null();
    }
    return eat_mem_initializer_list_2(ps);
}

ast_node eat_member_declaration_2(parse_state& ps) {
    // TODO:
    // attribute-specifier-seq(opt) decl-specifier-seq(opt) member-declarator-list(opt) ;
    // function-definition
    // using-declaration
    // static_assert-declaration
    // template-declaration
    // alias-declaration
    // empty-declaration

    {
        REWIND_SCOPE(member_decl);
        SYMBOL_CAPTURE_SCOPE(member_decl);

        bool has_attribs = eat_attribute_specifier_seq(ps);
        ast_node decl_spec_seq = eat_decl_specifier_seq_2(ps);
        ast_node member_decl_list = eat_member_declarator_list_2(ps);

        if (decl_spec_seq && !member_decl_list) {
            // TODO: Should technically throw if has_attribs
            ast::decl_specifier_seq* seq = decl_spec_seq.as<ast::decl_specifier_seq>();
            assert(seq);
            bool has_friend = seq->flags.has(e_friend);
            bool has_class_or_enum_specifier = false;
            bool has_elaborated_type_specifier = false;
            for (int i = 0; i < seq->specifiers.size(); ++i) {
                ast_node& spec = seq->specifiers[i];
                if (spec.is<ast::class_specifier>() || spec.is<ast::enum_specifier>()) {
                    has_class_or_enum_specifier = true;
                    break;
                }
                if (spec.is<ast::elaborated_type_specifier>()) {
                    has_elaborated_type_specifier = true;
                    break;
                }
            }

            if (has_friend && has_class_or_enum_specifier) {
                throw parse_exception("class or enum specifier can't be a part of a friend declaration", ps.get_latest_token());
            }

            if (has_friend || has_class_or_enum_specifier || has_elaborated_type_specifier) {
                expect(ps, ";");
                return ast_node::make<ast::member_declaration>(std::move(decl_spec_seq), ast_node::null());
            }
            REWIND_ON_EXIT(member_decl);
        } else if(member_decl_list) {
            if(accept(ps, ";")) {
                return ast_node::make<ast::member_declaration>(std::move(decl_spec_seq), std::move(member_decl_list));
            }
            auto list = member_decl_list.as<ast::member_declarator_list>();
            if (list->list.size() == 1 &&
                list->list[0].as<ast::member_function_declarator>()
            ) {
                ast_node body = eat_function_body_2(ps);
                return ast_node::make<ast::function_definition>(std::move(decl_spec_seq), std::move(list->list[0]), std::move(body));
            }

            REWIND_ON_EXIT(member_decl);
            SYMBOL_CAPTURE_CANCEL(member_decl);
        } else {
            REWIND_ON_EXIT(member_decl);
            SYMBOL_CAPTURE_CANCEL(member_decl);
        }
    }

    // Constructors, destructors and conversion functions
    {
        REWIND_SCOPE(member_decl);
        bool has_attribs = eat_attribute_specifier_seq(ps);
        ast_node member_decl_list = eat_member_declarator_list_2(ps);
        if (member_decl_list) {
            if (accept(ps, ";")) {
                return ast_node::make<ast::member_declaration>(ast_node::null(), std::move(member_decl_list));
            }
            auto list = member_decl_list.as<ast::member_declarator_list>();
            if (list->list.size() == 1 &&
                list->list[0].as<ast::member_function_declarator>()
            ) {
                ast_node body = eat_function_body_2(ps);
                return ast_node::make<ast::function_definition>(ast_node::null(), std::move(list->list[0]), std::move(body));
            }
        }
        REWIND_ON_EXIT(member_decl);
    }

    ast_node n = ast_node::null();
    /*
    ast_node n = ast_node::null();
    n = eat_function_definition_2(ps);
    if(n) return n;

    n = eat_using_declaration_2(ps);
    if(n) return n;
    */
    n = eat_static_assert_declaration_2(ps);
    if(n) return n;
    /*
    n = eat_template_declaration_2(ps);
    if(n) return n;

    n = eat_alias_declaration_2(ps);
    if(n) return n;

    n = eat_empty_declaration_2(ps);
    if(n) return n;
    */
    return ast_node::null();
}

ast_node eat_member_specification_one_2(parse_state& ps) {
    e_access_specifier access = e_access_unspecified;
    ast_node n = ast_node::null();
    if (eat_access_specifier_2(ps, access)) {
        expect(ps, ":");
        return ast_node::make<ast::member_access_spec>(access);
    } else if (n = eat_member_declaration_2(ps)) {
        return n;
    }
    return ast_node::null();
}

ast_node eat_member_specification_2(parse_state& ps) {
    ast_node n = eat_member_specification_one_2(ps);
    if(!n) return ast_node::null();

    ast_node member_spec = ast_node::make<ast::member_specification>();
    member_spec.as<ast::member_specification>()->push_back(std::move(n));

    while (true) {
        n = eat_member_specification_one_2(ps);
        if (!n) {
            break;
        }
        member_spec.as<ast::member_specification>()->push_back(std::move(n));
    }
    return member_spec;
}

ast_node eat_member_specification_limited_one_2(parse_state& ps) {
    e_access_specifier access = e_access_unspecified;
    attribute_specifier attr_spec;
    ast_node n = ast_node::null();
    if (eat_access_specifier_2(ps, access)) {
        expect(ps, ":");
        return ast_node::make<ast::member_access_spec>(access);
    } else if (eat_attribute_specifier_seq(ps, attr_spec)) {
        ps.no_reflect = attr_spec.find_attrib("no_reflect") != nullptr;

        if (attr_spec.find_attrib("cppi_class")) {
            expect(ps, ";");
            ast_node n = eat_class_specifier_2(ps);
            if(!n) throw parse_exception("cppi_class: expected a class-specifier", ps.peek_token());
            return n;
        } else if (attr_spec.find_attrib("cppi_enum")) {
            expect(ps, ";");
            ast_node n = eat_enum_specifier_2(ps);
            if(!n) throw parse_exception("cppi_enum: expected an enum-specifier", ps.peek_token());
            return n;
        } else if (attr_spec.find_attrib("cppi_decl")) {
            accept(ps, ";");
            ast_node n = eat_member_declaration_2(ps);
            if(!n) throw parse_exception("cppi_decl: expected a declaration", ps.peek_token());
            return n;
        }
        return ast_node::make<ast::dummy>();
    } else if (eat_balanced_token(ps)) {
        return ast_node::make<ast::dummy>();
    }

    return ast_node::null();
}

ast_node eat_member_specification_limited_2(parse_state& ps) {
    ast_node n = eat_member_specification_limited_one_2(ps);
    if(!n) return ast_node::null();

    ast_node member_spec = ast_node::make<ast::member_specification>();
    if(!n.is<ast::dummy>()) {
        member_spec.as<ast::member_specification>()->push_back(std::move(n));
    }

    while (true) {
        n = eat_member_specification_limited_one_2(ps);
        if (!n) {
            break;
        }
        if(!n.is<ast::dummy>()) {
            member_spec.as<ast::member_specification>()->push_back(std::move(n));
        }
    }
    return member_spec;
}

ast_node eat_class_specifier_2(parse_state& ps) {
    REWIND_SCOPE(class_specifier);
    SYMBOL_CAPTURE_SCOPE(class_specifier);

    symbol_ref sym = nullptr;
    ast_node class_head = ast_node::null();
    {
        e_class_key key;
        if (!eat_class_key_2(ps, key)) {
            return ast_node::null();
        }

        eat_attribute_specifier_seq(ps);
        ast_node class_head_name = eat_class_head_name_sem(ps);

        if (class_head_name && !class_head_name.is<ast::identifier>()) {
            throw parse_exception("TODO: only identifier as class-head-name supported", ps.get_latest_token());
        }

        std::string name;
        if (class_head_name) {
            name = class_head_name.as<ast::identifier>()->tok.str;
            sym = class_head_name.as<ast::identifier>()->sym;
        }

        if (!sym) {
            sym = ps.create_symbol<symbol_class>(0, name.c_str());
        }

        bool is_final = false;
        {
            REWIND_SCOPE(class_virt_specifier);
            if (class_head_name && accept(ps, "final")) {
                if (peek(ps, ":")) {
                    is_final = true;
                } else {
                    REWIND_ON_EXIT(class_virt_specifier);
                }
            }
        }

        ast_node base_clause = eat_base_clause_2(ps);

        class_head = ast_node::make<ast::class_head>(key, std::move(class_head_name), std::move(base_clause));
    }

    if (!accept(ps, "{")) {
        REWIND_ON_EXIT(class_specifier);
        SYMBOL_CAPTURE_CANCEL(class_specifier);
        return ast_node::null();
    }

    if (sym && sym->is_defined()) {
        throw parse_exception("class already defined", /* TODO: report the actual class-name */ps.get_latest_token());
    }

    auto class_scope = ps.enter_scope(scope_class);
    class_scope->set_owner(sym);
    sym->nested_symbol_table = class_scope;

    ast_node member_spec = ast_node::null();
    if(ps.is_limited()) {
        member_spec = eat_member_specification_limited_2(ps);
    } else {
        member_spec = eat_member_specification_2(ps);
    }

    expect(ps, "}");
    sym->set_defined(true);
    ps.exit_scope();

    ast_node class_specifier = ast_node::make<ast::class_specifier>(std::move(class_head), std::move(member_spec));
    return class_specifier;
}

