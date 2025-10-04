#include "resolve.hpp"


void resolve_namespace(parse_state& ps, ast::namespace_definition* nspace) {
    dbg_printf_color("namespace\n", DBG_WHITE);
    if (!nspace->decl_seq) {
        return;
    }
    resolve_declaration_seq(ps, nspace->decl_seq.as<ast::declaration_seq>());
}

void resolve_declaration_seq(parse_state& ps, ast::declaration_seq* seq) {
    for (int i = 0; i < seq->seq.size(); ++i) {
        ast_node& ast_decl = seq->seq[i];
        resolve_choose(ps, ast_decl);
    }
}

std::shared_ptr<symbol> resolve_class_name(parse_state& ps, ast::identifier* ident) {
    // TODO:
    return ps.get_current_scope()->lookup(ident->tok.str, LOOKUP_FLAG_CLASS);
}

std::shared_ptr<symbol> resolve_class_name(parse_state& ps, ast::simple_template_id* stid) {
    // TODO:
    return nullptr;
}

std::shared_ptr<symbol> resolve_class_name(parse_state& ps, ast::nested_name_specifier* nns) {
    // TODO:
    return nullptr;
}

std::shared_ptr<symbol> resolve_class_name(parse_state& ps, ast_node& name) {
    if (!name) {
        // Unnamed class
        // TODO: Should still return a symbol?
        return nullptr;
    }
    ast::identifier* ident = nullptr;
    ast::simple_template_id* stid = nullptr;
    ast::nested_name_specifier* nns = nullptr;

    if (ident = name.as<ast::identifier>()) {
        return resolve_class_name(ps, ident);
    } else if (stid = name.as<ast::simple_template_id>()) {
        return resolve_class_name(ps, stid);
    } else if (nns = name.as<ast::nested_name_specifier>()) {
        return resolve_class_name(ps, nns);
    }
    return nullptr;
}

void resolve_class_specifier(parse_state& ps, ast::class_specifier* spec) {
    dbg_printf_color("class_spec\n", DBG_WHITE);
    ast::class_head* head = spec->head.as<ast::class_head>();
    ast::member_specification* member_spec = spec->member_spec_seq.as<ast::member_specification>();

    if (!head->class_head_name) {
        dbg_printf_color("\tclass_spec: TODO: anonymous classes not handled for now\n", DBG_RED | DBG_GREEN);
        return;
    }
    if (!head->class_head_name.is<ast::identifier>()) {
        dbg_printf_color("\tclass_spec: TODO: nested names not handled for now\n", DBG_RED | DBG_GREEN);
        return;
    }

    ast::identifier* ident = head->class_head_name.as<ast::identifier>();

    std::shared_ptr<symbol_class> sym(new symbol_class);
    sym->name = ident->tok.str;
    sym->file = ps.get_latest_token().file;
    sym->defined = true;
    ps.add_symbol(sym);

    dbg_printf_color("\t%s\n", DBG_WHITE, sym->name.c_str());

    auto scope = ps.enter_scope(scope_class);
    sym->nested_symbol_table = scope;
    scope->set_owner(sym);

    // TODO: There's a lot
    
    ps.exit_scope();
}

void resolve_enum_specifier(parse_state& ps, ast::enum_specifier* spec) {
    dbg_printf_color("enum_spec\n", DBG_WHITE);
    ast::enum_head* head = spec->head.as<ast::enum_head>();
    ast::enumerator_list* enum_list = spec->enum_list.as<ast::enumerator_list>();
    bool is_scoped = head->key == ck_enum_class || head->key == ck_enum_struct;
    
    if (!head->name) {
        dbg_printf_color("\tenum_spec: TODO: anonymous enums not handled for now\n", DBG_RED | DBG_GREEN);
        return;
    }
    if (!head->name.is<ast::identifier>()) {
        dbg_printf_color("\tenum_spec: TODO: nested names not handled for now\n", DBG_RED | DBG_GREEN);
        return;
    }

    ast::identifier* name = head->name.as<ast::identifier>();
    std::shared_ptr<symbol> sym(new symbol_enum);
    sym->name = name->tok.str;
    sym->file = ps.get_latest_token().file;
    sym->defined = true;
    ps.add_symbol(sym);

    dbg_printf_color("\t%s\n", DBG_WHITE, sym->name.c_str());

    // TODO: enum base

    //auto parent_scope = ps.get_current_scope();
    auto scope = ps.enter_scope(scope_enum);
    sym->nested_symbol_table = scope;
    scope->set_owner(sym);

    if (enum_list) {
        for (int i = 0; i < enum_list->list.size(); ++i) {
            auto e = enum_list->list[i].as<ast::enumerator_definition>();
            if (!e) {
                dbg_printf_color("\texpected enumerator_definition, got '%s'\n", DBG_RED, enum_list->list[i].as<ast::node>()->get_type().name());
                assert(false);
                continue;
            }
            std::shared_ptr<symbol_enumerator> esym(new symbol_enumerator);
            esym->name = e->ident.as<ast::identifier>()->tok.str;
            esym->file = ps.get_latest_token().file;
            dbg_printf_color("\t%s\n", DBG_WHITE, esym->name.c_str());

            ps.add_symbol(esym);
            if (!is_scoped) {
                // TODO: Add to parent scope as well
            }
        }
    }
    ps.exit_scope();
}

void resolve_simple_declaration(parse_state& ps, ast::simple_declaration* decl) {
    dbg_printf_color("simple-declaration\n", DBG_WHITE);
}

void resolve_function_definition(parse_state& ps, ast::function_definition* def) {
    dbg_printf_color("function-definition\n", DBG_WHITE);
}

void resolve_choose(parse_state& ps, ast_node& n) {
    ast::declaration_seq* decl_seq = nullptr;
    ast::class_specifier* class_spec = nullptr;
    ast::enum_specifier* enum_spec = nullptr;
    ast::simple_declaration* decl = nullptr;
    ast::function_definition* func_def = nullptr;
    ast::namespace_definition* nspace = nullptr;

    if (decl_seq = n.as<ast::declaration_seq>()) {
        resolve_declaration_seq(ps, decl_seq);
    } else if (class_spec = n.as<ast::class_specifier>()) {
        resolve_class_specifier(ps, class_spec);
    } else if (enum_spec = n.as<ast::enum_specifier>()) {
        resolve_enum_specifier(ps, enum_spec);
    } else if (decl = n.as<ast::simple_declaration>()) {
        resolve_simple_declaration(ps, decl);
    } else if (func_def = n.as<ast::function_definition>()) {
        resolve_function_definition(ps, func_def);
    } else if (nspace = n.as<ast::namespace_definition>()) {
        resolve_namespace(ps, nspace);
    } else {
        dbg_printf_color("resolve_choose: unhandled ast node '%s'\n", DBG_WHITE, n.as<ast::node>()->get_type().name());
        //n.dbg_print(0);
        //dbg_printf_color("\n", DBG_WHITE);
    }
}

void resolve(parse_state& ps, ast_node& n) {
    dbg_printf_color("Walking the AST to fill the symbol table and resolve names\n", DBG_WHITE);
    resolve_choose(ps, n);
}
