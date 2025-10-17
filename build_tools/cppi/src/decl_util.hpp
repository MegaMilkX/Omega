#pragma once

#include "parse_state.hpp"
#include "ast/ast.hpp"
#include "type_id/type_id.hpp"
#include "template_argument.hpp"


const type_id_2* resolve_type_id(parse_state& ps, const ast::parameter_declaration* param);
const type_id_2* resolve_type_id(parse_state& ps, const ast::type_id* tid);

// Get symbol specifically with intention to redeclare or define it
inline symbol_ref get_symbol_for_redeclaration(parse_state& ps, const std::string& local_name, int lookup_flags) {
    auto scope = ps.get_current_scope();

    if (scope->get_type() == scope_template && !scope->get_owner()) {
        // Special case
        // If no owner - we're in the process
        // of template declaration.
        // Find an existing template declaration
        // and perform lookup inside that instead
        auto enclosing_scope = scope->get_enclosing();
        auto sym = enclosing_scope->get_symbol(local_name, lookup_flags | LOOKUP_FLAG_TEMPLATE);
        if (!sym) {
            return nullptr;
        }
        if (!sym->is<symbol_template>()) {
            throw parse_exception("template name already declared as something else", ps.get_latest_token());
        }
        symbol_template* tpl_sym = dynamic_cast<symbol_template*>(sym.get());
        // TODO: Compare and merge template parameters
        if (scope->template_parameters.size() != tpl_sym->template_parameters.size()) {
            throw parse_exception("template parameter count mismatch", ps.get_latest_token());
        }

        for (int i = 0; i < tpl_sym->template_parameters.size(); ++i) {
            const template_parameter& src = scope->template_parameters[i];
            template_parameter& dst = tpl_sym->template_parameters[i];
            
            if (src.get_kind() != dst.get_kind()) {
                throw parse_exception("template parameter kind mismatch", ps.get_latest_token());
            }
            
            if (src.has_default_arg() && dst.has_default_arg()) {
                throw parse_exception("template parameter default argument conflict", ps.get_latest_token());
            }

            if (src.has_default_arg()) {
                dst.set_default_arg(src);
            }
        }

        tpl_sym->validate_parameters();

        ps.exit_scope();
        ps.enter_scope(tpl_sym->nested_symbol_table);
        return tpl_sym->template_entity_sym;
    } else {
        return scope->get_symbol(local_name, lookup_flags);
    }
}

inline std::string get_declarator_name(parse_state& ps, const ast_node& declarator) {
    auto decltor = declarator.as<ast::declarator>();
    if (!decltor) {
        throw parse_exception("invalid declarator", ps.get_latest_token());
    }

    // TODO: Handle qualified-id

    if (!decltor->decl) {
        return "";
    }

    auto ident = decltor->decl.as<ast::identifier>();
    if (!ident) {
        throw parse_exception("TODO: only identifier allowed as declarator name", ps.get_latest_token());
    }

    return ident->tok.str;
}

inline void handle_parameter_declaration(parse_state& ps, const ast::parameter_declaration* param_decl) {
    auto scope = ps.get_current_scope();
    auto scope_type = scope->get_type();
    if (scope_type == scope_template) {
        std::string name = get_declarator_name(ps, param_decl->declarator);

        // TODO: handle is_variadic
        // declarator::is_variadic

        int lookup_flags = 
            LOOKUP_FLAG_TEMPLATE_PARAMETER;
        auto sym = scope->get_symbol(name, lookup_flags);
        if(sym) {
            throw parse_exception("template parameter redefinition", ps.get_latest_token());
        }

        auto sym_param = ps.create_symbol<symbol_template_parameter>(scope, name.c_str());
        sym_param->as<symbol_template_parameter>()->kind = e_template_parameter_non_type;
        sym_param->as<symbol_template_parameter>()->param_index = scope->template_parameters.size();

        const type_id_2* tid = resolve_type_id(ps, param_decl);

        eval_value default_arg;
        if (param_decl->initializer) {
            const expression* expr = param_decl->initializer.as<expression>();
            if (!expr) {
                throw parse_exception("TODO: only expressions allowed as default template parameter arguments", ps.get_latest_token());
            }
            eval_result res = expr->evaluate();
            if (!res) {
                throw parse_exception("failed to evaluate template parameter default argument", ps.get_latest_token());
            }
            default_arg = res.value;
        }

        // TODO: IS_VARIADIC
        scope->template_parameters.push_back(
            template_parameter::make_non_type(tid, default_arg, false)
        );
    } else {
        dbg_printf_color("TODO: parameter declaration not implemented in scope '%s'\n", DBG_YELLOW, scope_type_to_string(scope_type));
    }
}

inline void handle_type_parameter_declaration(parse_state& ps, const ast::type_parameter* param) {
    auto ident = param->ident.as<ast::identifier>();
    if (!ident) {
        throw parse_exception("type parameter requires an identifier", ps.get_latest_token());
    }

    auto scope = ps.get_current_scope();
    auto scope_type = scope->get_type();
    if (scope_type == scope_template) {
        std::string name = ident->tok.str;

        int lookup_flags = 
            LOOKUP_FLAG_TEMPLATE_PARAMETER;
        auto sym = scope->get_symbol(name, lookup_flags);
        if(sym) {
            throw parse_exception("template type parameter redefinition", ps.get_latest_token());
        }

        auto sym_param = ps.create_symbol<symbol_template_parameter>(scope, name.c_str());
        sym_param->as<symbol_template_parameter>()->kind = e_template_parameter_type;
        sym_param->as<symbol_template_parameter>()->param_index = scope->template_parameters.size();

        const type_id_2* default_tid = resolve_type_id(ps, param->initializer.as<ast::type_id>());

        scope->template_parameters.push_back(
            template_parameter::make_type(default_tid, param->is_variadic)
        );
    } else [[unlikely]] {
        throw parse_exception("type parameter in an unexpected scope", ps.get_latest_token());
    }
}

inline type_id_graph_node* resolve_type_id_declarator_impl(parse_state& ps, type_id_graph_node* base, const ast_node& part) {
    if (!part) {
        return base;
    }

    const ast::declarator_part* dpart = part.as<ast::declarator_part>();
    if (!dpart) {
        throw parse_exception("declarator part failed to cast to ast::declarator_part", ps.get_latest_token());
    }

    type_id_graph_node* n = base;
    if (dpart->next) {
        n = resolve_type_id_declarator_impl(ps, base, dpart->next);
    }
    
    if (const ast::ptr_declarator* ptr_decl = part.as<ast::ptr_declarator>()) {
        const ast::ptr_operator* op = ptr_decl->ptr_op.as<ast::ptr_operator>();
        if (!op) [[unlikely]] {
            throw parse_exception("implementation error: ptr_declarator has no operator node", ps.get_latest_token());
        }

        switch (op->type) {
        case e_ptr: n = type_id_storage::get()->walk_to_pointer(n, op->is_const, op->is_volatile); break;
        case e_ref: n = type_id_storage::get()->walk_to_ref(n); break;
        case e_rvref: n = type_id_storage::get()->walk_to_rvref(n); break;
        [[unlikely]] default:
            throw parse_exception("implementation error: unexpected ptr_declarator operator", ps.get_latest_token());
        }

        if (!n) [[unlikely]] {
            throw parse_exception("implementation error: type-id building resulted in null node", ps.get_latest_token());
        }
    } else if (const ast::parameters_and_qualifiers* func = part.as<ast::parameters_and_qualifiers>()) {
        const ast::parameter_declaration_list* param_list = func->param_decl_list.as<ast::parameter_declaration_list>();
        std::vector<const type_id_2*> params;
        if (param_list) {
            for (int i = 0; i < param_list->params.size(); ++i) {
                const ast_node& ast_param = param_list->params[i];
                const ast::parameter_declaration* param = ast_param.as<ast::parameter_declaration>();
                if (!param) [[unlikely]] {
                    throw parse_exception("parameter-declaration null or unexpected type of ast::node", ps.get_latest_token());
                }
                const type_id_2* type_id = resolve_type_id(ps, param);
                if (!type_id) [[unlikely]] {
                    throw parse_exception("failed to get type-id for parameter", ps.get_latest_token());
                }
                params.push_back(type_id);
            }
        }
        n = type_id_storage::get()->walk_to_function(n, params.data(), params.size(), func->is_const, func->is_volatile);
    } else if (const ast::array_declarator* array_ = part.as<ast::array_declarator>()) {
        const expression* expr = array_->const_expr.as<expression>();
        // TODO: expr can be optional, handle that?
        if (!expr) [[unlikely]] {
            throw parse_exception("TODO: [] without constant expression not supported", ps.get_latest_token());
        }
        eval_result res = expr->evaluate();
        if (res.result != e_eval_ok) {
            throw parse_exception("failed to evaluate array size expression", ps.get_latest_token());
        }
        if (!res.value.is_integral()) {
            throw parse_exception("array size must be of integral type", ps.get_latest_token());
        }
        // TODO: convert properly
        size_t sz = res.value.u64;
        n = type_id_storage::get()->walk_to_array(n, sz);
    } else [[unlikely]] {
        throw parse_exception("implementation error: unhandled type-id node", ps.get_latest_token());
    }

    return n;
}

inline type_id_graph_node* resolve_type_id_declarator(parse_state& ps, type_id_graph_node* base, const ast::declarator* declarator) {
    if (!declarator) {
        return base;
    }
    return resolve_type_id_declarator_impl(ps, base, declarator->next);
}
/*
inline const type_id_2* resolve_type_id(parse_state& ps, decl_flags decl_spec, const ast::declarator* declarator) {
    type_id_graph_node* n = type_id_storage::get()->get_base_node(decl_spec);
    n = resolve_type_id_declarator(ps, n, declarator);

    if (!n) [[unlikely]] {
        throw parse_exception("implementation error: type-id node cannot be null", ps.get_latest_token());
    }
    return n->type_id.get();
}*/

inline type_id_graph_node* resolve_user_defined_type(parse_state& ps, decl_flags flags, const ast_node* specifiers, int count) {
    assert(flags.has(e_decl_user_defined));

    type_id_graph_node* n = nullptr;
    for (int i = 0; i < count; ++i) {
        const ast_node& spec = specifiers[i];
        
        const ast::resolved_type_id* rtid
            = spec.as<ast::resolved_type_id>();
        if (rtid) {
            n = rtid->tid->get_lookup_node();
            break;
        }
        
        const ast::user_defined_type_specifier* udspec
            = spec.as<ast::user_defined_type_specifier>();
        if (!udspec) {
            const ast::nested_name_specifier* nns
                = spec.as<ast::nested_name_specifier>();
            if (!nns) {
                continue;
            }

            const ast_node& name = nns->get_qualified_name();
            if (!name) {
                throw parse_exception("implementation error: nested_name_specifier has no qualified name", ps.get_latest_token());
            }
            udspec = name.as<ast::user_defined_type_specifier>();
            if (!udspec) {
                throw parse_exception("nested_name_specifier does not denote a type", ps.get_latest_token());
            }
        }

        symbol_ref sym = udspec->resolve_to_symbol(ps);
        assert(sym);

        if (auto sym_typedef = sym->as<symbol_typedef>()) {
            n = sym_typedef->tid->get_lookup_node();
        } else {
            n = type_id_storage::get()->get_base_node(sym, flags.has(e_cv_const), flags.has(e_cv_volatile));
        }
        break;
    }

    if (!n) [[unlikely]] {
        throw parse_exception("failed to resolve user defined type from decl specifiers (probably the specifier used to denote a user defined type is unsupported)", ps.get_latest_token());
    }

    return n;
}

inline const type_id_2* resolve_type_id(parse_state& ps, const ast::parameter_declaration* param) {
    const ast::decl_specifier_seq* dsseq = param->decl_spec.as<ast::decl_specifier_seq>();
    if (!dsseq) {
        throw parse_exception("parameter_declaration missing decl_specifier_seq", ps.get_latest_token());
    }

    // declarator is optional
    const ast::declarator* declarator = param->declarator.as<ast::declarator>();
    
    type_id_graph_node* n = nullptr;
    if (dsseq->flags.has(e_decl_user_defined)) {
        const auto& seq = dsseq->specifiers;
        n = resolve_user_defined_type(ps, dsseq->flags, dsseq->specifiers.data(), dsseq->specifiers.size());
    } else if(dsseq->flags.has(e_decl_auto)) {
        throw parse_exception("auto not supported", ps.get_latest_token());
    } else if (dsseq->flags.has(e_decl_decltype)) {
        throw parse_exception("decltype not supported", ps.get_latest_token());
    } else {
        n = type_id_storage::get()->get_base_node(dsseq->flags);
    }

    if (!n) [[unlikely]] {
        throw parse_exception("implementation error: type-id node cannot be null", ps.get_latest_token());
    }

    n = resolve_type_id_declarator(ps, n, declarator);

    return n->type_id.get();
    //return resolve_type_id(ps, dsseq->flags, declarator);
}

inline const type_id_2* resolve_type_id(parse_state& ps, const ast::type_id* tid) {
    if (!tid) {
        return nullptr;
    }
    const ast::type_specifier_seq* tsseq = tid->type_spec_seq.as<ast::type_specifier_seq>();
    if (!tsseq) {
        throw parse_exception("type_id missing type_specifier_seq", ps.get_latest_token());
    }

    // declarator is optional
    const ast::declarator* declarator = tid->declarator_.as<ast::declarator>();

    type_id_graph_node* n = nullptr;
    if (tsseq->flags.has(e_decl_user_defined)) {
        const auto& seq = tsseq->seq;
        n = resolve_user_defined_type(ps, tsseq->flags, tsseq->seq.data(), tsseq->seq.size());
    } else if(tsseq->flags.has(e_decl_auto)) {
        throw parse_exception("auto not supported", ps.get_latest_token());
    } else if (tsseq->flags.has(e_decl_decltype)) {
        throw parse_exception("decltype not supported", ps.get_latest_token());
    } else {
        n = type_id_storage::get()->get_base_node(tsseq->flags);
    }

    if (!n) [[unlikely]] {
        throw parse_exception("implementation error: type-id node cannot be null", ps.get_latest_token());
    }

    n = resolve_type_id_declarator(ps, n, declarator);

    return n->type_id.get();
    //return resolve_type_id(ps, tsseq->flags, declarator);
}

inline const type_id_2* resolve_type_id(parse_state& ps, const ast::decl_specifier_seq* dsseq, const ast::declarator* pdeclarator) {
    type_id_graph_node* n = nullptr;
    if (dsseq->flags.has(e_decl_user_defined)) {
        const auto& seq = dsseq->specifiers;
        n = resolve_user_defined_type(ps, dsseq->flags, seq.data(), seq.size());
    } else if (dsseq->flags.has(e_decl_auto)) {
        // TODO: resolve underlying type from initializer
        throw parse_exception("TODO: auto for init_declarator", ps.get_latest_token());
    } else if (dsseq->flags.has(e_decl_decltype)) {
        throw parse_exception("decltype not supported", ps.get_latest_token());
    } else {
        n = type_id_storage::get()->get_base_node(dsseq->flags);
    }

    if (!n) [[unlikely]] {
        throw parse_exception("implementation error: type-id node cannot be null", ps.get_latest_token());
    }

    n = resolve_type_id_declarator(ps, n, pdeclarator);
    return n->type_id.get();
}

inline const type_id_2* resolve_type_id(parse_state& ps, const ast::decl_specifier_seq* dsseq, const ast::member_declarator* decl) {
    const ast::declarator* pdeclarator = decl->declarator.as<ast::declarator>();
    if (!pdeclarator) {
        throw parse_exception("declarator null or not a declarator", ps.get_latest_token());
    }

    return resolve_type_id(ps, dsseq, pdeclarator);
}
/*
inline const type_id_2* resolve_type_id(parse_state& ps, const ast::decl_specifier_seq* dsseq, const ast::init_declarator* decl) {
    const ast::declarator* pdeclarator = decl->declarator.as<ast::declarator>();
    if (!pdeclarator) {
        throw parse_exception("declarator null or not a declarator", ps.get_latest_token());
    }

    return resolve_type_id(ps, dsseq, pdeclarator);
}*/


inline symbol_ref declare_typedef(parse_state& ps, const ast::decl_specifier_seq* dsseq, const ast::declarator* declarator) {
    auto cur_scope = ps.get_current_scope();
    if (cur_scope->get_type() != scope_namespace && cur_scope->get_type() != scope_default) {
        throw parse_exception("implementation error: attempted to declare a namespace scope typedef outside of namespace scope", ps.get_latest_token());
    }

    const ast::identifier* ident = declarator->decl.as<ast::identifier>();
    if (!ident) {
        throw parse_exception("TODO: only identifier is supported as declarator-id for a namespace scope typedef", ps.get_latest_token());
    }

    const type_id_2* tid = resolve_type_id(ps, dsseq, declarator);

    auto sym = cur_scope->get_symbol(ident->tok.str, LOOKUP_FLAG_TYPEDEF);
    if (sym && sym->as<symbol_typedef>()->tid != tid) {
        throw parse_exception("typedef redefinition with a different type", ident->tok);
    }
    
    if (sym) {
        return sym;
    }

    sym = ps.get_current_scope()->create_symbol<symbol_typedef>(ident->tok.str.c_str(), ident->tok.file, ps.no_reflect);
    sym->as<symbol_typedef>()->tid = tid;
    sym->set_defined(true);
    return sym;
}

inline symbol_ref declare_function(parse_state& ps, const ast::decl_specifier_seq* dsseq, const ast::declarator* declarator) {
    auto cur_scope = ps.get_current_scope();
    if (cur_scope->get_type() != scope_namespace && cur_scope->get_type() != scope_default) {
        throw parse_exception("implementation error: attempted to declare a namespace scope function outside of namespace scope", ps.get_latest_token());
    }

    const ast::identifier* ident = declarator->decl.as<ast::identifier>();
    if (!ident) {
        throw parse_exception("TODO: only identifier is supported as declarator-id for a namespace scope function", ps.get_latest_token());
    }

    const type_id_2* tid = resolve_type_id(ps, dsseq, declarator);

    auto sym_func = ps.get_current_scope()->get_symbol(ident->tok.str, LOOKUP_FLAG_FUNCTION);
    if (!sym_func) {
        sym_func = ps.get_current_scope()->create_symbol<symbol_function>(ident->tok.str.c_str(), ident->tok.file, ps.no_reflect);
    }
    auto sym_overload = sym_func->as<symbol_function>()->get_or_create_overload(tid);

    return sym_overload;
}

inline symbol_ref declare_object(parse_state& ps, const ast::decl_specifier_seq* dsseq, const ast::declarator* declarator) {
    auto cur_scope = ps.get_current_scope();
    if (cur_scope->get_type() != scope_namespace && cur_scope->get_type() != scope_default) {
        throw parse_exception("implementation error: attempted to declare a namespace scope object outside of namespace scope", ps.get_latest_token());
    }

    const ast::object_name* object_name = declarator->decl.as<ast::object_name>();
    if (object_name) {
        throw parse_exception("variable redefinition", ps.get_latest_token());
    }

    const ast::identifier* ident = declarator->decl.as<ast::identifier>();
    if (!ident) {
        throw parse_exception("name already declared as a different kind of symbol", ps.get_latest_token());
    }

    assert(dsseq);
    decl_flags flags = dsseq->flags;
    if (flags.has(e_storage_static)) {
        throw parse_exception("static namespace scope declarations not supported", ps.get_latest_token());
    }
    if (flags.has(e_storage_thread_local)) {
        throw parse_exception("thread_local namespace scope declarations not supported", ps.get_latest_token());
    }

    const type_id_2* tid = resolve_type_id(ps, dsseq, declarator);

    /*
    auto sym = cur_scope->get_symbol(ident->tok.str, LOOKUP_FLAG_OBJECT);
    if (sym && sym->is_defined()) {
        throw parse_exception("object redefinition", ident->tok);
    }*/
    auto sym = ps.get_current_scope()->create_symbol<symbol_object>(ident->tok.str.c_str(), ident->tok.file, ps.no_reflect);
    sym->as<symbol_object>()->tid = tid;
    if(!flags.has(e_storage_extern)) {
        sym->set_defined(true);
    }
    return sym;
}


inline symbol_ref declare_enumerator(parse_state& ps, const ast::identifier* ident, const expression* expr) {
    if (ps.get_current_scope()->get_type() != scope_enum) {
        throw parse_exception("implementation error: tried to declare an enumerator outside of enum scope", ps.get_latest_token());
    }

    auto cur_scope = ps.get_current_scope();
    auto sym = cur_scope->get_symbol(ident->tok.str, LOOKUP_FLAG_ENUMERATOR);
    if (sym) {
        throw parse_exception("enumerator redefinition", ps.get_latest_token());
    }

    symbol_enum* sym_enum = cur_scope->get_owner()->as<symbol_enum>();

    // TODO: Properly support different underlying types
    uint64_t i = sym_enum->next_enumerator_value;
    if (expr) {
        eval_result res = expr->evaluate();
        if (!res) {
            throw parse_exception("failed to evaluate an enumerator initializer expression", ps.get_latest_token());
        }
        if (!res.value.is_integral()) {
            throw parse_exception("enumerator initializer must be of integral type", ps.get_latest_token());
        }
        i = res.value.u64;
    }
    sym_enum->next_enumerator_value = i + 1;

    sym = cur_scope->create_symbol<symbol_enumerator>(ident->tok.str, ident->tok.file, ps.no_reflect);
    sym->set_defined(true);
    sym->as<symbol_enumerator>()->value = i;

    if (sym_enum->is_unscoped()) {
        auto encl_scope = cur_scope->get_enclosing();
        auto sym = encl_scope->create_symbol<symbol_enumerator>(ident->tok.str, ident->tok.file, ps.no_reflect);
        sym->set_defined(true);
        sym->as<symbol_enumerator>()->value = i;
    }

    return sym;
}


inline symbol_ref declare_bit_field(parse_state& ps, const ast::decl_specifier_seq* dsseq, const ast::declarator* declarator, const expression* expr) {
    if (ps.get_current_scope()->get_type() != scope_class) {
        throw parse_exception("implementation error: tried to declare a bit field outside of class scope", ps.get_latest_token());
    }
    throw parse_exception("bit fields are not supported", ps.get_latest_token());
    /*
    // TODO:
    return symbol_ref();*/
}

inline symbol_ref declare_member_typedef(parse_state& ps, const ast::decl_specifier_seq* dsseq, const ast::declarator* declarator) {
    if (ps.get_current_scope()->get_type() != scope_class) {
        throw parse_exception("implementation error: tried to declare a member typedef outside of class scope", ps.get_latest_token());
    }

    if (!declarator) [[unlikely]] {
        throw parse_exception("declarator is required for a member typedef", ps.get_latest_token());
    }
    if (!dsseq) [[unlikely]] {
        throw parse_exception("decl_specifier_seq is required for a member typedef", ps.get_latest_token());
    }

    decl_flags flags = dsseq->flags;
    if (flags.has(e_decl_auto)) {
        throw parse_exception("auto not allowed in a typedef", ps.get_latest_token());
    }
    if (flags.has(e_decl_decltype)) {
        throw parse_exception("TODO: decltype not supported for member typedef", ps.get_latest_token());
    }

    const ast::identifier* ident = declarator->decl.as<ast::identifier>();
    if (!ident) {
        throw parse_exception("only plain identifier supported for a member typedef declaration", ps.get_latest_token());
    }

    auto sym = ps.get_current_scope()->get_symbol(ident->tok.str, LOOKUP_FLAG_TYPEDEF | LOOKUP_FLAG_OBJECT | LOOKUP_FLAG_FUNCTION);
    if (sym) {
        if (!sym->is<symbol_typedef>()) {
            throw parse_exception("member redefinition as a different kind of symbol", ps.get_latest_token());
        }
        throw parse_exception("member object redefinition", ident->tok);
    }

    sym = ps.get_current_scope()->create_symbol<symbol_typedef>(ident->tok.str.c_str(), ident->tok.file, ps.no_reflect);
    const type_id_2* tid = resolve_type_id(ps, dsseq, declarator);
    sym->as<symbol_typedef>()->tid = tid;
    sym->set_defined(true);
    return sym;
}

inline symbol_ref declare_member_function(parse_state& ps, const ast::decl_specifier_seq* dsseq, const ast::declarator* declarator, e_virt_flags virt_flags, bool is_pure) {
    if (ps.get_current_scope()->get_type() != scope_class) {
        throw parse_exception("implementation error: tried to declare a member function outside of class scope", ps.get_latest_token());
    }

    if (!declarator) [[unlikely]] {
        throw parse_exception("declarator is required for a member function", ps.get_latest_token());
    }

    decl_flags flags;
    if(dsseq) {
        flags = dsseq->flags;
    } else {
        flags.add(e_decl_void);
    }

    if (flags.has(e_decl_auto)) {
        throw parse_exception("TODO: auto not supported for member functions", ps.get_latest_token());
    }
    if (flags.has(e_decl_decltype)) {
        throw parse_exception("TODO: decltype not supported for for member functions", ps.get_latest_token());
    }

    if (flags.has(e_storage_static)) {
        throw parse_exception("TODO: static member functions not supported", ps.get_latest_token());
    }
    if (flags.has(e_storage_thread_local)) {
        throw parse_exception("thread_local is illegal for a return type", ps.get_latest_token());
    }

    symbol_ref sym_func = nullptr;
    const ast::function_name* fn_name = nullptr;
    const ast::identifier* ident = nullptr;
    if (fn_name = declarator->decl.as<ast::function_name>()) {
        sym_func = fn_name->sym;
    } else {
        ident = declarator->decl.as<ast::identifier>();
    }

    if (!fn_name && !ident) {
        throw parse_exception("member redefinition as a different kind of symbol", ps.get_latest_token());
    }
    /*
    const ast::identifier* ident = declarator->decl.as<ast::identifier>();
    if (!ident) {
        throw parse_exception("only plain identifier supported for a member function declaration", ps.get_latest_token());
    }

    auto sym_func = ps.get_current_scope()->get_symbol(ident->tok.str, LOOKUP_FLAG_TYPEDEF | LOOKUP_FLAG_OBJECT | LOOKUP_FLAG_FUNCTION);
    if (sym_func && !sym_func->is<symbol_function>()) {
        throw parse_exception("member redefinition as a different kind of symbol", ps.get_latest_token());
    }*/
    if (!sym_func) {
        sym_func = ps.get_current_scope()->create_symbol<symbol_function>(ident->tok.str, ident->tok.file, ps.no_reflect);
    }

    const type_id_2* tid = nullptr;
    if(dsseq) {
        tid = resolve_type_id(ps, dsseq, declarator);
    } else {
        auto n = type_id_storage::get()->get_base_node(flags);
        n = resolve_type_id_declarator(ps, n, declarator);
        tid = n->type_id.get();
    }
    auto sym_overload = sym_func->as<symbol_function>()->get_or_create_overload(tid);
    return sym_overload;
}

inline symbol_ref declare_member_object(parse_state& ps, const ast::decl_specifier_seq* dsseq, const ast::declarator* declarator) {
    if (ps.get_current_scope()->get_type() != scope_class) {
        throw parse_exception("implementation error: tried to declare a member outside of class scope", ps.get_latest_token());
    }

    if (!declarator) [[unlikely]] {
        throw parse_exception("declarator is required for a member declaration", ps.get_latest_token());
    }
    if (!dsseq) [[unlikely]] {
        throw parse_exception("decl_specifier_seq is required for a member declaration", ps.get_latest_token());
    }

    decl_flags flags = dsseq->flags;
    if (flags.has(e_decl_auto)) {
        throw parse_exception("auto not supported for class member objects", ps.get_latest_token());
    }
    if (flags.has(e_decl_decltype)) {
        throw parse_exception("TODO: decltype not supported for class member objects", ps.get_latest_token());
    }

    if (flags.has(e_storage_static)) {
        throw parse_exception("TODO: static member objects not supported", ps.get_latest_token());
    } else if (flags.has(e_storage_thread_local)) {
        throw parse_exception("thread_local is illegal for non-static member objects", ps.get_latest_token());
    }

    const ast::identifier* ident = declarator->decl.as<ast::identifier>();
    if (!ident) {
        throw parse_exception("only plain identifier supported for a member declaration", ps.get_latest_token());
    }

    auto sym = ps.get_current_scope()->get_symbol(ident->tok.str, LOOKUP_FLAG_TYPEDEF | LOOKUP_FLAG_OBJECT | LOOKUP_FLAG_FUNCTION);
    if (sym) {
        if (!sym->is<symbol_object>()) {
            throw parse_exception("member redefinition as a different kind of symbol", ps.get_latest_token());
        }
        throw parse_exception("member object redefinition", ident->tok);
    }

    sym = ps.get_current_scope()->create_symbol<symbol_object>(ident->tok.str.c_str(), ident->tok.file, ps.no_reflect);
    const type_id_2* tid = resolve_type_id(ps, dsseq, declarator);
    sym->as<symbol_object>()->tid = tid;
    sym->set_defined(true);
    return sym;
}

inline symbol_ref instantiate_simple_template_id(parse_state& ps, const ast_node& n);
inline void specify_base_class(parse_state& ps, symbol_class* sym_class, const ast::base_specifier* bs) {
    if (bs->is_dependent()) {
        return;
    }

    if (const ast::class_name* cn = bs->base_type_spec.as<ast::class_name>()) {
        sym_class->base_classes.push_back(cn->sym);
        return;
    }

    if (const ast::simple_template_id* stid = bs->base_type_spec.as<ast::simple_template_id>()) {
        symbol_ref sym = instantiate_simple_template_id(ps, bs->base_type_spec);
        if (!sym->is<symbol_class>()) {
            throw parse_exception("class base must be a class", ps.get_latest_token());
        }
        sym_class->base_classes.push_back(sym);
        return;
    }

    if (const ast::resolved_type_id* rtid = bs->base_type_spec.as<ast::resolved_type_id>()) {
        const type_id_2* tid = rtid->tid;
        const type_id_2_user_type* tidudt = tid->as<type_id_2_user_type>();
        if (!tidudt) {
            throw parse_exception("base specifier should be a class type", ps.get_latest_token());
        }
        // TODO: maybe check for cv too
        symbol_class* sym_base = tidudt->sym->as<symbol_class>();
        if (!sym_base) {
            throw parse_exception("base specifier expected to be a class", ps.get_latest_token());
        }
        sym_class->base_classes.push_back(tidudt->sym);
        return;
    }

    throw parse_exception("unexpected base specifier", ps.get_latest_token());
}


inline symbol_ref get_template_instance(parse_state& ps, symbol_ref tpl_sym, const ast_node& args) {
    if (!tpl_sym) {
        throw parse_exception("missing template symbol", ps.get_latest_token());
    }

    auto arg_list = args.as<ast::template_argument_list>();
    /*
    dbg_printf_color("TODO: Getting template instance ", DBG_YELLOW);
    tpl_sym->dbg_print();
    dbg_printf_color("\n", DBG_WHITE);
    */
    symbol_template* ptpl_sym = tpl_sym->as<symbol_template>();
    std::vector<template_argument> arguments(ptpl_sym->template_parameters.size());
    const auto& tparams = ptpl_sym->template_parameters;

    int n_params_fulfilled = 0;
    if(arg_list) {
        //passed_arg_count = arg_list->list.size();

        for (int i = 0; i < arg_list->list.size(); ++i) {
            const auto& arg = arg_list->list[i];
        
            if (i >= tparams.size()) [[unlikely]] {
                throw parse_exception("implementation error: template argument overflow", ps.get_latest_token());
            }

            if (tparams[i].is_pack()) {
                break;
            }

            if (tparams[i].get_kind() == e_template_parameter_type) {
                const ast::type_id* tid = arg.as<ast::type_id>();
                if (!tid) {
                    throw parse_exception("expected a type template argument", ps.get_latest_token());
                }
                const type_id_2* type_id = resolve_type_id(ps, tid);
                type_id = type_id->strip_cv();
                arguments[i] = template_argument::make_type(type_id);
            } else if (tparams[i].get_kind() == e_template_parameter_non_type) {
                const expression* expr = arg.as<expression>();
                if (!expr) {
                    throw parse_exception("expected a non-type template argument", ps.get_latest_token());
                }
                // TODO: IMPORTANT! use parameter type, not argument type
                // TODO: check if result is valid
                // TODO: check if result converts to template parameter type
                // TODO: enum types should be handled here too
                eval_result res = expr->evaluate();
                arguments[i] = template_argument::make_non_type(res.value);
            } else {
                throw parse_exception("TODO: template parameter kind not supported", ps.get_latest_token());
            }

            ++n_params_fulfilled;
        }

        // Any argument overflow goes into a variadic pack
        int pack_idx = n_params_fulfilled;
        if(pack_idx < tparams.size() && tparams[pack_idx].is_pack()) {
            std::vector<template_argument> arg_pack(arg_list->list.size() - pack_idx);

            if (tparams[pack_idx].get_kind() == e_template_parameter_type) {
                arguments[pack_idx] = template_argument::make_pack(e_template_argument_type, std::vector<template_argument>());
            } else if (tparams[pack_idx].get_kind() == e_template_parameter_non_type) {
                arguments[pack_idx] = template_argument::make_pack(e_template_argument_non_type, std::vector<template_argument>());
            } else {
                throw parse_exception("template parameter kind not supported", ps.get_latest_token());
            }

            if (tparams[pack_idx].get_kind() == e_template_parameter_type) {
                for (int i = pack_idx; i < arg_list->list.size(); ++i) {
                    int pack_i = i - pack_idx;
                    const auto& arg = arg_list->list[i];

                    const ast::type_id* tid = arg.as<ast::type_id>();
                    if (!tid) {
                        throw parse_exception("expected a type template argument in an argument pack", ps.get_latest_token());
                    }
                    const type_id_2* type_id = resolve_type_id(ps, tid);
                    type_id = type_id->strip_cv();
                    arg_pack[pack_i] = template_argument::make_type(type_id);                
                }
                arguments[pack_idx] = template_argument::make_pack(e_template_argument_type, arg_pack);
                ++n_params_fulfilled;
            } else if (tparams[pack_idx].get_kind() == e_template_parameter_non_type) {
                for (int i = pack_idx; i < arg_list->list.size(); ++i) {
                    int pack_i = i - pack_idx;
                    const auto& arg = arg_list->list[i];
                    const expression* expr = arg.as<expression>();
                    if (!expr) {
                        throw parse_exception("expected a non-type template argument", ps.get_latest_token());
                    }
                    // TODO: IMPORTANT! use parameter type, not argument type
                    // TODO: check if result is valid
                    // TODO: check if result converts to template parameter type
                    eval_result res = expr->evaluate();
                    arg_pack[pack_i] = template_argument::make_non_type(res.value);
                }
                arguments[pack_idx] = template_argument::make_pack(e_template_argument_non_type, arg_pack);
                ++n_params_fulfilled;
            } else {
                throw parse_exception("TODO: unsupported template parameter kind", ps.get_latest_token());
            }
        }
    }
    
    for (int i = n_params_fulfilled; i < tpl_sym->as<symbol_template>()->template_parameters.size(); ++i) {
        auto& tpl_param = tpl_sym->as<symbol_template>()->template_parameters[i];
        if (tpl_param.is_pack()) {
            break;
        }
        if (!tpl_param.has_default_arg()) {
            throw parse_exception("template argument to parameter count mismatch", ps.get_latest_token());
        }
        arguments[i] = tpl_param.get_default_arg();
    }

    // Build template instance internal name
    std::string tpl_local_internal_name = tpl_sym->get_local_internal_name();
    std::string internal_name = tpl_local_internal_name + "I";
    for (int i = 0; i < arguments.size(); ++i) {
        const auto& arg = arguments[i];
        // TODO: non-type arguments should already be casted
        // to parameter type at this point
        internal_name += arg.to_internal_name();
    }
    internal_name += "E";

    //dbg_printf_color("template instance internal name %s\n", DBG_YELLOW, internal_name.c_str());

    int lookup_flags = 0;
    if (ptpl_sym->template_entity_sym->is<symbol_class>()) {
        lookup_flags = LOOKUP_FLAG_CLASS;
    }

    if (lookup_flags == 0) {
        throw parse_exception("tried to lookup an unsupported template instantiation", ps.get_latest_token());
    }

    // TODO: if resolving nested-name-specifier -> get_symbol() in a specific scope
    // TODO: otherwise -> lookup() in current scope
    symbol_ref sym_instance = ps.get_current_scope()->lookup(internal_name, lookup_flags);
    if (sym_instance) {
        return sym_instance;
    }

    // Build normalized as-in-code name (for code generation)
    std::string norm_source_name = tpl_sym->get_local_source_name() + "<";
    for (int i = 0; i < arguments.size(); ++i) {
        const auto& arg = arguments[i];
        if (arg.is_empty_pack()) {
            continue;
        }

        if(i) norm_source_name += ", ";
        norm_source_name += arg.to_source_name();
    }
    norm_source_name += ">";

    // Instantiate
    if (ptpl_sym->template_entity_sym->is<symbol_class>()) {
        auto tpl_encl_scope = tpl_sym->nested_symbol_table->get_enclosing();
        // TODO: unsure which no_reflect is relevant, templated entity's or the template's
        sym_instance = tpl_encl_scope->create_symbol<symbol_class>(
            norm_source_name.c_str(),
            internal_name.c_str(),
            internal_name.c_str(),
            ptpl_sym->template_entity_sym->file,
            ptpl_sym->no_reflect
        );

        //dbg_printf_color("template instance source name %s\n", DBG_YELLOW, sym_instance->get_source_name().c_str());

        ps.enter_scope(tpl_encl_scope);
        auto class_scope = ps.enter_scope(scope_class);
        sym_instance->nested_symbol_table = class_scope;
        class_scope->set_owner(sym_instance);
        ps.exit_scope();
        ps.exit_scope();
        
        {
            assert(ptpl_sym->template_class_pattern);
            const ast::node* tpl_pattern = ptpl_sym->template_class_pattern;

            ast_node class_specifier(tpl_pattern->clone());
            ast::class_specifier* class_spec = class_specifier.as<ast::class_specifier>();//tpl_pattern->as<ast::class_specifier>();
            
            ast_node& base_clause = class_spec->head.as<ast::class_head>()->base_clause;
            
            base_clause = std::move(base_clause.resolve_dependent(arguments.data(), arguments.size()));
            {
                const ast::base_specifier_list* bsl = base_clause.as<ast::base_specifier_list>();
                for (int i = 0; bsl && i < bsl->list.size(); ++i) {
                    specify_base_class(ps, sym_instance->as<symbol_class>(), bsl->list[i].as<ast::base_specifier>());
                }
            }
            
            class_spec->member_spec_seq = std::move(class_spec->member_spec_seq.resolve_dependent(arguments.data(), arguments.size()));
            {
                // TODO: actually declare members
            }

            printf("\nInstantiation: \n");
            class_specifier.dbg_print();
            printf("\n");
        }

        if (ptpl_sym->template_entity_sym->is_defined()) {
            sym_instance->set_defined(true);
        }
    } else {
        throw parse_exception("tried to instantiate an unsupported template declaration", ps.get_latest_token());
    }

    return sym_instance;
}

inline symbol_ref instantiate_simple_template_id(parse_state& ps, const ast::simple_template_id* stid) {
    /*
    if (stid->sym_instance) {
        // instance has already been resolved,
        // will probably not ever be hit though,
        // since backtracking and reparsing rebuilds the ast::simple_template_id,
        // it's not reused
        return;
    }*/

    auto ident = stid->ident.as<ast::identifier>();
    if (!ident) {
        throw parse_exception("simple_template_id missing an identifier", ps.get_latest_token());
    }

    auto tpl_sym = ident->sym;

    if (!tpl_sym) {
        throw parse_exception("simple_template_id identifier was not resolved", ps.get_latest_token());
    }
    if (!tpl_sym->is<symbol_template>()) {
        throw parse_exception("simple_template_id references a non-template symbol", ps.get_latest_token());
    }

    //stid->sym_instance = get_template_instance(ps, tpl_sym, stid->template_argument_list);
    return get_template_instance(ps, tpl_sym, stid->template_argument_list);
}

inline symbol_ref instantiate_simple_template_id(parse_state& ps, const ast_node& n) {
    auto stid = n.as<ast::simple_template_id>();
    if (!stid) {
        throw parse_exception("handle_simple_template_id: node is null or not a simple_template_id", ps.get_latest_token());
    }
    return instantiate_simple_template_id(ps, stid);
}


inline void register_template_class_pattern(parse_state& ps, const ast_node& class_specifier) {
    if (ps.get_current_scope()->get_type() != scope_template) {
        return;
    }

    symbol_ref sym = ps.get_current_scope()->get_owner();
    if (!sym) {
        throw parse_exception("register_template_pattern() called before template symbol is created", ps.get_latest_token());
    }

    symbol_template* tpl_sym = sym->as<symbol_template>();
    if (!tpl_sym->template_entity_sym) {
        throw parse_exception("register_template_pattern() called before templated entity symbol is set", ps.get_latest_token());
    }

    if (!tpl_sym->template_entity_sym->is<symbol_class>()) {
        throw parse_exception("register_template_pattern called for a class while template is not a class", ps.get_latest_token());
    }

    tpl_sym->template_class_pattern = class_specifier.as<ast::class_specifier>();
}