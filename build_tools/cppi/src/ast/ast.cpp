#include "ast.hpp"

#include "symbol.hpp"
#include "decl_util.hpp"

namespace ast {

bool class_name::is_dependent() const {
    return sym->is<symbol_template_parameter>();
}
void class_name::dbg_print(int indent) const {
    dbg_printf_color_indent("%s", indent, DBG_USER_DEFINED_TYPE, sym->get_local_source_name().c_str());
}
void enum_name::dbg_print(int indent) const {
    dbg_printf_color_indent("%s", indent, DBG_USER_DEFINED_TYPE, sym->get_local_source_name().c_str());
}

eval_result enumerator_name::evaluate() const {
    // TODO: support different underlying types!
    return eval_value(e_int, sym->as<symbol_enumerator>()->value);
}
void enumerator_name::dbg_print(int indent) const {
    dbg_printf_color_indent("%s", indent, DBG_LITERAL, sym->get_local_source_name().c_str());
}

void typedef_name::dbg_print(int indent) const {
    dbg_printf_color_indent("%s", indent, DBG_USER_DEFINED_TYPE, sym->get_local_source_name().c_str());
}
void function_name::dbg_print(int indent) const {
    dbg_printf_color_indent("%s", indent, DBG_WHITE, sym->get_local_source_name().c_str());
}
void object_name::dbg_print(int indent) const {
    dbg_printf_color_indent("%s", indent, DBG_WHITE, sym->get_local_source_name().c_str());
}

ast_node dependent_type_name::resolve_dependent(const template_argument* args, int arg_count) const {
    symbol_template_parameter* tplparam = sym->as<symbol_template_parameter>();
    if(!tplparam) [[unlikely]] {
        assert(false);
        return ast_node::null();
    }

    int pidx = tplparam->param_index;
    if (pidx < 0 || pidx >= arg_count) [[unlikely]] {
        assert(false);
        return ast_node::null();
    }

    const template_argument& arg = args[pidx];
    if (arg.get_kind() != e_template_argument_type) {
        throw std::exception("template argument expected to be a type for a dependent type name");
    }

    const type_id_2* tid = arg.get_contained_type();
    
    return ast_node::make<ast::resolved_type_id>(tid);
}
void dependent_type_name::dbg_print(int indent) const {
    dbg_printf_color_indent("%s", indent, DBG_USER_DEFINED_TYPE, sym->get_source_name().c_str());
}

ast_node dependent_non_type_name::resolve_dependent(const template_argument* args, int arg_count) const {
    symbol_template_parameter* tplparam = sym->as<symbol_template_parameter>();
    if (!tplparam) [[unlikely]] {
         assert(false);
         return ast_node::null();
    }

    int pidx = tplparam->param_index;
    if (pidx < 0 || pidx >= arg_count) [[unlikely]] {
        assert(false);
        return ast_node::null();
    }

    const template_argument& arg = args[pidx];
    if (arg.get_kind() != e_template_argument_non_type) {
        throw std::exception("template argument expected to be a non type for a dependent non type name");
    }

    eval_value val = arg.get_contained_value();
    return ast_node::make<ast::resolved_const_expr>(val);
}
void dependent_non_type_name::dbg_print(int indent) const {
    dbg_printf_color_indent("%s", indent, DBG_USER_DEFINED_TYPE, sym->get_source_name().c_str());
}

void namespace_name::dbg_print(int indent) const {
    dbg_printf_color_indent("%s", indent, DBG_WHITE, sym->get_source_name().c_str());
}

symbol_ref simple_template_id::resolve_to_symbol(parse_state& ps) const {
    if (sym_instance) {
        return sym_instance;
    }
    sym_instance = instantiate_simple_template_id(ps, this);
    return sym_instance;
}


std::shared_ptr<symbol_table> nested_name_specifier::get_qualified_scope(parse_state& ps) const {
    if (next) {
        return next.as<ast::nested_name_specifier>()->get_qualified_scope(ps);
    }
    if (auto namespace_name = n.as<ast::namespace_name>()) {
        return namespace_name->sym->nested_symbol_table;
    }
    if (auto class_name = n.as<ast::class_name>()) {
        return class_name->sym->nested_symbol_table;
    }
    if (auto enum_name = n.as<ast::enum_name>()) {
        return enum_name->sym->nested_symbol_table;
    }
    if (auto stid = n.as<ast::simple_template_id>()) {
        // simple template id should already be instantiated here,
        // but calling resolve_to_symbol just to be safe anyway
        return stid->resolve_to_symbol(ps)->nested_symbol_table;
    }
    throw parse_exception("failed to get qualified scope from nested_name_specifier chain", ps.get_latest_token());
    return nullptr;
}
const ast_node& nested_name_specifier::get_qualified_name() const {
    if (auto nns = next.as<ast::nested_name_specifier>()) {
        return nns->get_qualified_name();
    }
    // TODO: meh
    return next;
}

}

