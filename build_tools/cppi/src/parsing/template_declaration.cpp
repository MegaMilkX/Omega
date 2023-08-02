#include "template_declaration.hpp"

#include "common.hpp"
#include "declaration.hpp"
#include "decl_specifier.hpp"
#include "type_specifier.hpp"
#include "function_definition.hpp"
#include "balanced_token.hpp"
#include "primary_expression.hpp"
#include "attribute_specifier.hpp"
#include "enum_specifier.hpp"

bool eat_template_parameter_list(parse_state& ps, tpl_parameter_list& param_list);


bool eat_type_parameter(parse_state& ps, tpl_parameter_list& param_list) {
    if (eat_keyword(ps, kw_class) || eat_keyword(ps, kw_typename)) {
        if(param_list.has_pack_param()) {
            throw parse_exception("Template pack parameter must appear last in a template parameter list", ps.next_token());
        }
        bool must_have_default_arg = param_list.has_default_args();
        bool is_pack = accept(ps, "...");
        token tok;
        eat_token(ps, tt_identifier, &tok);

        tpl_parameter param;
        param.name = tok.str;
        param.param_type = is_pack ? TPL_PARAM_PACK : TPL_PARAM_TYPE;
        if (!is_pack && accept(ps, "=")) {
            if (!eat_type_id(ps, param.default_type_arg)) {
                throw parse_exception("Expected type_id", ps.next_token());
            }
        } else if(!is_pack && must_have_default_arg) {
            throw parse_exception("Template parameter after a paramter with default argument must also have a default argument", ps.next_token());
        }
        param_list.add(param);

        std::shared_ptr<symbol_typedef> sym(new symbol_typedef);
        sym->name = tok.str;
        ps.add_symbol(sym);
        return true;
    }
    if (eat_keyword(ps, kw_template)) {
        // TODO:
        throw parse_exception("Template template parameters are not implemented", ps.next_token());
        /*
        expect(ps, "<");
        if (!eat_template_parameter_list(ps, param_list)) {
            throw parse_exception("Expected a template parameter_list", ps.next_token());
        }
        expect(ps, ">");
        expect(ps, kw_class);
        bool is_pack = accept(ps, "...");
        token tok;
        eat_token(ps, tt_identifier, &tok);
        if (!is_pack && accept(ps, "=")) {
            if (!eat_id_expression(ps)) {
                throw parse_exception("Expected id-expression", tok);
            }
            return true;
        }
        return true;*/
    }
    return false;
}
static bool eat_parameter_declaration(parse_state& ps, tpl_parameter_list& param_list) {
    // TODO:
    return false;
}
bool eat_template_parameter(parse_state& ps, tpl_parameter_list& param_list) {
    if (eat_type_parameter(ps, param_list)) {
        return true;
    }
    if (eat_parameter_declaration(ps, param_list)) {
        return true;
    }
    return false;
}
bool eat_template_parameter_list(parse_state& ps, tpl_parameter_list& param_list) {
    if (!eat_template_parameter(ps, param_list)) {
        return false;
    }
    while (true) {
        if (!accept(ps, ",")) {
            return true;
        }
        if (!eat_template_parameter(ps, param_list)) {
            throw parse_exception("Expected a template parameter", ps.next_token());
        }
        continue;
    }
}

bool eat_tpl_function_specifier(parse_state& ps, decl_spec& ds, decl_type& dt) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_keyword) {
        ps.rewind_();
        return false;
    }
    if (tok.kw_type == kw_inline) {
        if (!ds.is_decl_compatible(DECL_FN_INLINE)) {
            throw parse_exception("Incompatible function specifier", tok);
        }
        ds.set_decl_flag(DECL_FN_INLINE, COMPAT_FN_INLINE);
        ps.pop_rewind_point();
        return true;
    }
    if (tok.kw_type == kw_virtual) {
        throw parse_exception("virtual not allowed for templates", tok);
    }
    if (tok.kw_type == kw_explicit) {
        if (!ds.is_decl_compatible(DECL_FN_EXPLICIT)) {
            throw parse_exception("Incompatible function specifier", tok);
        }
        ds.set_decl_flag(DECL_FN_EXPLICIT, COMPAT_FN_EXPLICIT);
        ps.pop_rewind_point();
        return true;
    }
    ps.rewind_();
    return false;
}
bool eat_tpl_enum_head(parse_state& ps) {
    e_enum_key key;
    if (!eat_enum_key(ps, key)) {
        return false;
    }
    eat_attribute_specifier_seq(ps);
    std::shared_ptr<symbol_table> scope;
    if (eat_nested_name_specifier(ps, scope)) {
        if (!eat_identifier(ps)) {
            throw parse_exception("Expected an identifier", ps.next_token());
        }
        if (eat_enum_base(ps)) {
            throw parse_exception("enum templates not allowed", ps.next_token());
        }
    } else {
        eat_identifier(ps);
        if (eat_enum_base(ps)) {
            throw parse_exception("enum templates not allowed", ps.next_token());
        }
    }
}
bool eat_tpl_enum_specifier(parse_state& ps) {
    if (!eat_tpl_enum_head(ps)) {
        return false;
    }
    ps.push_rewind_point();
    if (!accept(ps, "{")) {
        ps.rewind_();
        return false;
    }

    throw parse_exception("enum templates not allowed", ps.next_token());

    return false;
}
bool eat_tpl_class_head_name(parse_state& ps, std::shared_ptr<symbol>& out_sym) {
    std::shared_ptr<symbol_table> nns_scope;
    eat_nested_name_specifier(ps, nns_scope);
    token tok;
    bool has_name = eat_token(ps, tt_identifier, &tok);
    if (!has_name && nns_scope) {
        throw parse_exception("Missing class name", tok);
    }
    if (!nns_scope) {
        nns_scope = ps.get_current_scope();
    }
    std::string class_name;
    std::shared_ptr<symbol> sym;
    if (has_name) {
        class_name = tok.str;
        sym = nns_scope->get_symbol(class_name, LOOKUP_FLAG_TEMPLATE);
        if (sym && !sym->is<symbol_class>()) {
            throw parse_exception("Template name already declared as a different entity", tok);
        }
        if (sym) {
            out_sym = sym;
        } else {
            out_sym = std::shared_ptr<symbol>(new symbol_class);
            out_sym->name = class_name;
            ps.add_symbol(out_sym);
        }
        out_sym->file = tok.file;
    } else {
        throw parse_exception("Template class name required", tok);
    }
    return true;
}

bool eat_tpl_base_type_specifier(parse_state& ps) {
    // TODO: 
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_identifier) {
        ps.rewind_();
        return false;
    }
    // TODO: class-or-decltype

    ps.pop_rewind_point();
    return true;
}
bool eat_tpl_base_specifier(parse_state& ps) {
    eat_attribute_specifier_seq(ps);
    token tok;
    if (eat_tpl_base_type_specifier(ps)) {
        return true;
    }
    else if (eat_token(ps, "virtual", &tok)) {
        eat_access_specifier(ps);
        if (!eat_tpl_base_type_specifier(ps)) {
            throw parse_exception("Expected a base_type_specifier", ps.next_token());
        }
        return true;
    }
    else if (eat_access_specifier(ps)) {
        eat_token(ps, "virtual", &tok);
        if (!eat_tpl_base_type_specifier(ps)) {
            throw parse_exception("Expected a base_type_specifier", ps.next_token());
        }
        return true;
    }
    else {
        return false;
    }
}
bool eat_tpl_base_specifier_list(parse_state& ps) {
    if (!eat_tpl_base_specifier(ps)) {
        return false;
    }
    token tok;
    eat_token(ps, "...", &tok);
    while (true) {
        ps.push_rewind_point();
        token tok = ps.next_token();
        if (tok.str != ",") {
            ps.rewind_();
            return true;
        }
        if (!eat_tpl_base_specifier(ps)) {
            throw parse_exception("Expected a base_specifier", tok);
        }
        eat_token(ps, "...", &tok);
        ps.pop_rewind_point();
    }
}
bool eat_tpl_base_clause(parse_state& ps) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.str != ":") {
        ps.rewind_();
        return false;
    }
    if (!eat_tpl_base_specifier_list(ps)) {
        throw parse_exception("Expected a base_specifier_list", tok);
    }
    ps.pop_rewind_point();
    return true;
}
bool eat_tpl_class_virt_specifier(parse_state& ps) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_identifier) {
        ps.rewind_();
        return false;
    }
    if (tok.str != "final") {
        ps.rewind_();
        return false;
    }
    ps.pop_rewind_point();
    return true;
}
bool eat_tpl_class_head(parse_state& ps, std::shared_ptr<symbol>& out_sym) {
    e_class_key key;
    if (!eat_class_key(ps, key)) {
        return false;
    }
    eat_attribute_specifier_seq(ps);

    if (eat_tpl_class_head_name(ps, out_sym)) {
        eat_tpl_class_virt_specifier(ps);
        eat_tpl_base_clause(ps);
    } else {
        eat_tpl_base_clause(ps);
    }
    return true;
}
std::shared_ptr<symbol> eat_tpl_class_specifier(parse_state& ps) {
    ps.push_rewind_point();

    std::shared_ptr<symbol> sym;
    if (!eat_tpl_class_head(ps, sym)) {
        return 0;
    }
    if (!accept(ps, "{")) {
        ps.rewind_();
        return 0;
    }

    assert(sym);
    if (sym->is_defined()) {
        throw parse_exception("Template class already defined", ps.next_token());
    }
    auto class_scope = ps.enter_scope();
    class_scope->set_owner(sym);
    
    // TODO: template class members
    eat_balanced_token_seq(ps);

    expect(ps, "}");
    sym->nested_symbol_table = class_scope;
    sym->set_defined(true);

    ps.exit_scope();
    ps.pop_rewind_point();
    return sym;
}
std::shared_ptr<symbol> eat_tpl_elaborated_type_specifier(parse_state& ps) {
    e_class_key key;
    if (eat_class_key(ps, key)) {
        std::shared_ptr<symbol_table> scope;
        if (eat_nested_name_specifier(ps, scope)) {
            if (eat_keyword(ps, kw_template)) {
                std::shared_ptr<symbol> sym;
                if (!eat_simple_template_id(ps, sym)) {
                    throw parse_exception("Expected a simple_template_id", ps.next_token());
                }
                // TODO:
                return sym;
            }
            std::shared_ptr<symbol> sym;
            if (eat_simple_template_id(ps, sym, scope)) {
                // TODO:
                return sym;
            }
            
            sym = eat_class_name(ps, scope);
            if (!sym) {
                sym = eat_template_name(ps, scope);
            }
            if (!sym) {
                token tok;
                if (!eat_token(ps, tt_identifier, &tok)) {
                    throw parse_exception("Expected an identifier", tok);
                }
                sym.reset(new symbol_template);
                sym->name = tok.str;
                sym->file = tok.file;
                ps.add_symbol(sym);
                return sym;
            } else {
                return sym;
            }
        }
        eat_attribute_specifier_seq(ps);
        eat_nested_name_specifier(ps, scope);
        std::shared_ptr<symbol> sym = eat_class_name(ps, scope);
        if (!sym) {
            sym = eat_template_name(ps, scope);
        }
        if (!sym) {
            token tok;
            if (!eat_token(ps, tt_identifier, &tok)) {
                throw parse_exception("Expected an identifier", tok);
            }
            sym.reset(new symbol_template);
            sym->name = tok.str;
            sym->file = tok.file;
            ps.add_symbol(sym);
            return sym;
        } else {
            return sym;
        }
    } else if(eat_keyword(ps, kw_enum)) {
        std::shared_ptr<symbol_table> scope;
        eat_nested_name_specifier(ps, scope);
        std::shared_ptr<symbol> sym = eat_enum_name(ps, scope);
        if (!sym) {
            throw parse_exception("Expected enumeration name", ps.next_token());
        }
        return sym;
    }
    return 0;
}
bool eat_tpl_trailing_type_specifier(parse_state& ps, decl_spec& dspec, decl_type& dtype) {
    if (eat_simple_type_specifier(ps, &dtype)) {
        return true;
    }
    std::shared_ptr<symbol> sym;
    if (sym = eat_tpl_elaborated_type_specifier(ps)) {
        dtype.compatibility_flags = 0;
        dtype.type_flags = fl_user_defined;
        dtype.user_defined_type = sym;
        dtype.is_type_definition = false;
        return true;
    }
    if (eat_typename_specifier(ps)) {
        return true;
    }
    int cv_flag = 0;
    if (eat_cv_qualifier(ps, &cv_flag)) {
        if (!dspec.is_decl_compatible(cv_flag)) {
            // TODO: Now reporting next token after the specifier, fix this
            throw parse_exception("Incompatible cv specifier", ps.next_token());
        }
        dspec.set_decl_flag(cv_flag, ~0);
        return true;
    }
    return false;
}
bool eat_tpl_type_specifier(parse_state& ps, decl_spec& ds, decl_type& dt) {
    std::shared_ptr<symbol> type_sym;
    if (type_sym = eat_tpl_class_specifier(ps)) {
        if ((dt.compatibility_flags & fl_user_defined) == 0) {
            throw parse_exception("class-specifier is not valid here", ps.next_token());
        }
        dt.type_flags = fl_user_defined;
        dt.compatibility_flags = 0;
        dt.user_defined_type = type_sym;
        dt.is_type_definition = true;
        return true;
    }
    if (eat_tpl_enum_specifier(ps)) {
        // This will throw anyway if an enum definition is found, unreachable
        return true;
    }
    if (eat_tpl_trailing_type_specifier(ps, ds, dt)) {
        return true;
    }

    return false;
}
bool eat_tpl_storage_class_specifier(parse_state& ps, decl_spec& ds, decl_type& dt) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_keyword) {
        ps.rewind_();
        return false;
    }
    if (tok.kw_type == kw_register) {
        if (!ds.is_decl_compatible(DECL_STORAGE_REGISTER)) {
            throw parse_exception("Incompatible storage class specifier", tok);
        }
        ds.set_decl_flag(DECL_STORAGE_REGISTER, COMPAT_STORAGE_REGISTER);
        ps.pop_rewind_point();
        return true;
    }
    if (tok.kw_type == kw_static) {
        if (!ds.is_decl_compatible(DECL_STORAGE_STATIC)) {
            throw parse_exception("Incompatible storage class specifier", tok);
        }
        ds.set_decl_flag(DECL_STORAGE_STATIC, COMPAT_STORAGE_STATIC);
        ps.pop_rewind_point();
        return true;
    }
    if (tok.kw_type == kw_thread_local) {
        if (!ds.is_decl_compatible(DECL_STORAGE_THREAD_LOCAL)) {
            throw parse_exception("Incompatible storage class specifier", tok);
        }
        ds.set_decl_flag(DECL_STORAGE_THREAD_LOCAL, COMPAT_STORAGE_THREAD_LOCAL);
        ps.pop_rewind_point();
        return true;
    }
    if (tok.kw_type == kw_extern) {
        if (!ds.is_decl_compatible(DECL_STORAGE_EXTERN)) {
            throw parse_exception("Incompatible storage class specifier", tok);
        }
        ds.set_decl_flag(DECL_STORAGE_EXTERN, COMPAT_STORAGE_EXTERN);
        ps.pop_rewind_point();
        return true;
    }
    if (tok.kw_type == kw_mutable) {
        if (!ds.is_decl_compatible(DECL_STORAGE_MUTABLE)) {
            throw parse_exception("Incompatible storage class specifier", tok);
        }
        ds.set_decl_flag(DECL_STORAGE_MUTABLE, COMPAT_STORAGE_MUTABLE);
        ps.pop_rewind_point();
        return true;
    }
    ps.rewind_();
    return false;
}
bool eat_tpl_decl_specifier(parse_state& ps, decl_spec& ds, decl_type& dt) {
    if (eat_tpl_storage_class_specifier(ps, ds, dt)) {
        return true;
    }
    if (eat_tpl_type_specifier(ps, ds, dt)) {
        return true;
    }
    if (eat_tpl_function_specifier(ps, ds, dt)) {
        return true;
    }

    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type == tt_keyword) {
        if (tok.kw_type == kw_friend) {
            if (!ds.is_decl_compatible(DECL_FRIEND)) {
                throw parse_exception("Incompatible specifier", tok);
            }
            ds.set_decl_flag(DECL_FRIEND, COMPAT_FRIEND);
            ps.pop_rewind_point();
            return true;
        }
        if (tok.kw_type == kw_typedef) {
            if (!ds.is_decl_compatible(DECL_TYPEDEF)) {
                throw parse_exception("Incompatible specifier", tok);
            }
            ds.set_decl_flag(DECL_TYPEDEF, COMPAT_TYPEDEF);
            ps.pop_rewind_point();
            return true;
        }
        if (tok.kw_type == kw_constexpr) {
            if (!ds.is_decl_compatible(DECL_CONSTEXPR)) {
                throw parse_exception("Incompatible specifier", tok);
            }
            ds.set_decl_flag(DECL_CONSTEXPR, COMPAT_CONSTEXPR);
            ps.pop_rewind_point();
            return true;
        }
    }
    ps.rewind_();
    return false;
}
bool eat_tpl_decl_specifier_seq(parse_state& ps, decl_spec& ds, decl_type& dt) {
    if (!eat_tpl_decl_specifier(ps, ds, dt)) {
        return false;
    }
    while (eat_tpl_decl_specifier(ps, ds, dt)) {}
    eat_attribute_specifier_seq(ps);
    return true;
}
std::shared_ptr<symbol> eat_tpl_simple_declaration(parse_state& ps) {
    ps.push_rewind_point();
    if (eat_attribute_specifier_seq(ps)) {}

    decl_spec dspec;
    decl_type dtype;
    if (!eat_tpl_decl_specifier_seq(ps, dspec, dtype)) {
        ps.rewind_();
        return 0;
    }
    
    if (dtype.is_type_definition) {
        expect(ps, ";");
        ps.pop_rewind_point();
        return dtype.user_defined_type;
    } else {
        declarator d;
        std::shared_ptr<symbol> decl_sym;
        if (!eat_declarator(ps, dspec, dtype, &d, &decl_sym)) {
            if (dtype.user_defined_type) {
                expect(ps, ";");
                ps.pop_rewind_point();
                return dtype.user_defined_type;
            } else {
                throw parse_exception("Expected a declarator", ps.next_token());
            }
        }
        
        if (!d.is_func()) {
            decl_sym->set_defined(true);
            eat_initializer(ps);

            expect(ps, ";");
            ps.pop_rewind_point();
            return decl_sym;
        } else {
            if (eat_function_body(ps, &d)) {
                decl_sym->set_defined(true);

                ps.pop_rewind_point();
                return decl_sym;
            } else {
                expect(ps, ";");

                ps.pop_rewind_point();
                return decl_sym;
            }
        }
    }
}
std::shared_ptr<symbol> eat_tpl_declaration(parse_state& ps) {
    std::shared_ptr<symbol> sym;
    if (sym = eat_tpl_simple_declaration(ps)) {
        return sym;
    }
    return 0;
}
bool eat_template_declaration(parse_state& ps) {
    if (!eat_keyword(ps, kw_template)) {
        return false;
    }

    std::shared_ptr<symbol_table> tpl_scope = ps.enter_scope();

    tpl_parameter_list param_list;
    expect(ps, "<");
    if (!eat_template_parameter_list(ps, param_list)) {
        throw parse_exception("Expected a template_parameters_list", ps.next_token());
    }
    expect(ps, ">");

    std::shared_ptr<symbol> decl_sym = eat_tpl_declaration(ps);
    if (!decl_sym) {
        throw parse_exception("Expected a declaration", ps.next_token());
    }

    ps.exit_scope();

    std::shared_ptr<symbol> tpl_existing = ps.get_current_scope()->lookup(decl_sym->name, LOOKUP_FLAG_TEMPLATE);
    if (tpl_existing) {
        // TODO: Update existing template
    } else {
        std::shared_ptr<symbol_template> tpl_sym(new symbol_template);
        tpl_scope->set_owner(tpl_sym);
        tpl_sym->nested_symbol_table = tpl_scope;
        tpl_sym->name = decl_sym->name;
        tpl_sym->file = decl_sym->file;
        ps.add_symbol(tpl_sym);
    }

    return true;
}