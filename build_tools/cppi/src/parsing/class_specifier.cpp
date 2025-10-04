#include "class_specifier.hpp"
#include "common.hpp"
#include "attribute_specifier.hpp"
#include "function_definition.hpp"
#include "decl_specifier.hpp"
#include "primary_expression.hpp"
#include "type_specifier.hpp"
#include "enum_specifier.hpp"
#include "parse_exception.hpp"


bool eat_class_key(parse_state& ps, e_class_key& out_key) {
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
bool eat_class_virt_specifier(parse_state& ps) {
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

bool eat_class_head_name(parse_state& ps, std::shared_ptr<symbol>& out_sym) {
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

        sym = nns_scope->get_symbol(class_name, LOOKUP_FLAG_CLASS | LOOKUP_FLAG_ENUM);
        if (sym && !sym->is<symbol_class>()) {
            throw parse_exception("Name already declared as a different entity", tok);
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
        out_sym = std::shared_ptr<symbol>(new symbol_class);
        ps.add_symbol(out_sym);
    }    

    return true;
}
bool eat_base_type_specifier(parse_state& ps, std::shared_ptr<symbol>& type_sym) {
    ps.push_rewind_point();

    std::shared_ptr<symbol_table> scope;
    if (eat_nested_name_specifier(ps, scope)) {
        type_sym = eat_class_name(ps, scope);
        if (type_sym) {
            ps.pop_rewind_point();
            return true;
        } else {
            throw parse_exception("Expected a class name", ps.get_latest_token());
        }
    }
    type_sym = eat_class_name(ps, 0);
    if (type_sym) {
        ps.pop_rewind_point();
        return true;
    }

    if (eat_decltype_specifier(ps)) {
        // TODO:
        throw parse_exception("decltype as base type not implemented", ps.get_latest_token());
    }

    ps.rewind_();
    return false;
}
bool eat_access_specifier(parse_state& ps) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_keyword) {
        ps.rewind_();
        return false;
    }
    if (tok.str == "private") {
        ps.pop_rewind_point();
        return true;
    } else if(tok.str == "protected") {
        ps.pop_rewind_point();
        return true;
    } else if(tok.str == "public") {
        ps.pop_rewind_point();
        return true;
    }
    ps.rewind_();
    return false;
}
bool eat_base_specifier(parse_state& ps, std::shared_ptr<symbol>& out_sym) {
    eat_attribute_specifier_seq(ps);
    token tok;
    std::shared_ptr<symbol> base_type_sym;
    if (eat_base_type_specifier(ps, base_type_sym)) {
        assert(out_sym->is<symbol_class>());
        symbol_class* sym_class = (symbol_class*)out_sym.get();
        sym_class->base_classes.push_back(base_type_sym);
        return true;
    } else if(eat_token(ps, "virtual", &tok)) {
        eat_access_specifier(ps);
        if (!eat_base_type_specifier(ps, base_type_sym)) {
            throw parse_exception("Expected a base_type_specifier", ps.next_token());
        }
        assert(out_sym->is<symbol_class>());
        symbol_class* sym_class = (symbol_class*)out_sym.get();
        sym_class->base_classes.push_back(base_type_sym);
        return true;
    } else if(eat_access_specifier(ps)) {
        eat_token(ps, "virtual", &tok);
        if (!eat_base_type_specifier(ps, base_type_sym)) {
            throw parse_exception("Expected a base_type_specifier", ps.next_token());
        }
        assert(out_sym->is<symbol_class>());
        symbol_class* sym_class = (symbol_class*)out_sym.get();
        sym_class->base_classes.push_back(base_type_sym);
        return true;
    } else {
        return false;
    }
}
bool eat_base_specifier_list(parse_state& ps, std::shared_ptr<symbol>& out_sym) {
    if (!eat_base_specifier(ps, out_sym)) {
        return false;
    }
    token tok;
    if (eat_token(ps, "...", &tok)) {
        // TODO:
    }
    while (true) {
        ps.push_rewind_point();
        token tok = ps.next_token();
        if (tok.str != ",") {
            ps.rewind_();
            return true;
        }
        if (!eat_base_specifier(ps, out_sym)) {
            throw parse_exception("Expected a base_specifier", tok);
        }
        eat_token(ps, "...", &tok);
        ps.pop_rewind_point();
    }
}
bool eat_base_clause(parse_state& ps, std::shared_ptr<symbol>& out_sym) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.str != ":") {
        ps.rewind_();
        return false;
    }
    if (!eat_base_specifier_list(ps, out_sym)) {
        throw parse_exception("Expected a base_specifier_list", tok);
    }
    ps.pop_rewind_point();
    return true;
}
class eat_base_clause {

};
bool eat_class_head(parse_state& ps, std::shared_ptr<symbol>& out_sym) {
    e_class_key key;
    if (!eat_class_key(ps, key)) {
        return false;
    }
    attribute_specifier attr_spec;
    eat_attribute_specifier_seq(ps, attr_spec);

    if (eat_class_head_name(ps, out_sym)) {
        eat_class_virt_specifier(ps);
        eat_base_clause(ps, out_sym);
    } else {
        eat_base_clause(ps, out_sym);
    }
    out_sym->attrib_spec.merge(attr_spec);
    return true;
}
bool eat_initializer_clause(parse_state& ps) {
    if (eat_braced_init_list(ps)) {
        return true;
    }
    // TODO: assignment-expression
    return eat_balanced_token_seq_except(ps, { ",", ";" });
}
bool eat_brace_or_equal_initializer(parse_state& ps) {
    token tok;
    if (eat_token(ps, "=", &tok)) {
        if (!eat_initializer_clause(ps)) {
            throw parse_exception("Expected an initializer_clause", tok);
        }
        return true;
    }
    if (eat_braced_init_list(ps)) {
        return true;
    }
    return false;
}
static bool eat_pure_specifier(parse_state& ps) {
    ps.push_rewind_point();
    token tok;
    if (!eat_token(ps, "=", &tok)) {
        ps.pop_rewind_point();
        return false;
    }
    if (!eat_token(ps, "0", &tok)) {
        ps.rewind_();
        return false;
    }
    ps.pop_rewind_point();
    return true;
}

bool eat_member_declarator(parse_state& ps, decl_spec& dspec, decl_type& dtype, declarator* d) {
    eat_declarator(ps, dspec, dtype, d, 0); // bitfield identifier is optional

    if (d->is_func()) {
        bool a = eat_virt_specifier_seq(ps);
        a |= eat_pure_specifier(ps);
        if (a) {
            // Can't be anything else if at least one of these is present
            return true;
        }
    } else if(!d->name.empty()) {
        if (eat_brace_or_equal_initializer(ps)) {
            // Can't be anything else, returning
            return true;
        }
    }

    {
        // try bitfield
        ps.push_rewind_point();
        eat_attribute_specifier_seq(ps);
        token tok;
        if (eat_token(ps, ":", &tok)) {
            // TODO: CHeck that a bit field has integral type
            /*
            if (!d->decl_type_.empty()) {
                throw parse_exception("Invalid type for a bit field", tok);
            }*/
            eat_balanced_token_seq_except(ps, { ",", ";" });
            ps.pop_rewind_point();
            return true;
        } else {
            ps.rewind_();
        }
    }

    if (!d->name.empty()) {
        return true;
    } else {
        return false;
    }
}
bool eat_member_declarator_list(parse_state& ps, decl_spec& dspec, decl_type& dtype, std::vector<declarator>* declarations) {
    declarator d;
    if (!eat_member_declarator(ps, dspec, dtype, &d)) {
        return false;
    }
    declarations->push_back(std::move(d));
    while (true) {
        ps.push_rewind_point();
        token tok;
        if (!eat_token(ps, ",", &tok)) {
            ps.rewind_();
            return true;
        }
        declarator d;
        if (!eat_member_declarator(ps, dspec, dtype, &d)) {
            throw parse_exception("Expected a declarator", tok);
        }
        declarations->push_back(std::move(d));
        ps.pop_rewind_point();
    }
    return true;
}
bool eat_member_declaration(parse_state& ps, attribute_specifier& attr_spec = attribute_specifier()) {
    ps.push_rewind_point();

    attribute_specifier attr_spec_;
    eat_attribute_specifier_seq(ps, attr_spec_);
    decl_spec dspec;
    decl_type dtype;
    eat_decl_specifier_seq(ps, dspec, dtype);
    std::vector<declarator> declarations;
    if (dspec.has_decl_flag(DECL_TYPEDEF)) {
        if (!eat_declarator_list(ps, dspec, dtype, &declarations)) {
            throw parse_exception("Expected a declarator", ps.next_token());
        }
    } else {
        eat_member_declarator_list(ps, dspec, dtype, &declarations);
        if (declarations.size() == 1 && declarations[0].is_func()) {
            // Try function body
            if (eat_function_body(ps, &declarations[0])) {
                for (auto& decl : declarations) {
                    decl.sym->attrib_spec.merge(attr_spec);
                    decl.sym->attrib_spec.merge(attr_spec_);
                }
                ps.pop_rewind_point();
                return true;
            }
        }
        
    }
    if (accept(ps, ";")) {
        for (auto& decl : declarations) {
            decl.sym->attrib_spec.merge(attr_spec);
            decl.sym->attrib_spec.merge(attr_spec_);
        }
        ps.pop_rewind_point();
        return true;
    }
    ps.rewind_();
    // TODO:
    /*
        using-declaration
        static_assert-declaration
        template-declaration
        alias-declaration
        empty-declaration
    */
    return false;
}
bool eat_member_specification(parse_state& ps) {
    if (eat_member_declaration(ps)) {
        eat_member_specification(ps);
        return true;
    }
    if (eat_access_specifier(ps)) {
        expect(ps, ":");
        eat_member_specification(ps);
        return true;
    }
    return false;
}
bool eat_member_specification_limited(parse_state& ps) {
    while (true) {
        if (eat_access_specifier(ps)) {
            expect(ps, ":");
            // TODO: remember access specifier
            continue;
        }

        attribute_specifier attr_spec;
        if (eat_attribute_specifier_seq(ps, attr_spec)) {
            if (attr_spec.find_attrib("cppi_class")) {
                expect(ps, ";");
                if (!eat_class_specifier_limited(ps)) {
                    throw parse_exception("cppi_class attribute must be followed by a class definition", ps.latest_token);
                }
                continue;
            } else if(attr_spec.find_attrib("cppi_enum")) {
                expect(ps, ";");
                if (!eat_enum_specifier_limited(ps)) {
                    throw parse_exception("cppi_enum attribute must be followed by an enumeration definition", ps.latest_token);
                }
                continue;
            } else if (attr_spec.find_attrib("cppi_decl")) {
                eat_member_declaration(ps, attr_spec);
                continue;
            }

            continue;
        }

        if (!eat_balanced_token(ps)) {
            break;
        }
    }
    return true;
}

bool resolve_placeholders(parse_state& ps, type_id& tid) {
    if (tid.empty()) {
        return false;
    }
    auto last_part = tid.get_last();
    type_id_part_object* o = last_part->cast_to<type_id_part_object>();
    if (!o) {
        assert(false);
        return false;
    }
    if (o->object_type_.type_flags != fl_user_defined) {
        return false;
    }
    auto& udt = o->object_type_.user_defined_type;
    if (!udt) {
        assert(false);
        return false;
    }
    if (!udt->is<symbol_placeholder>()) {
        return false;
    }
    symbol_placeholder* phd = (symbol_placeholder*)udt.get();
    ps.push_tokens(phd->token_cache, false);
    decl_spec dspec;
    decl_type dtype;
    if (!eat_trailing_type_specifier(ps, dspec, dtype, false)) {
        throw parse_exception("Failed to parse class member type placeholder", ps.next_token());
    }
    if(dtype.type_flags == fl_user_defined) {
        std::shared_ptr<symbol>& sym = dtype.user_defined_type;
        if (sym->is<symbol_typedef>()) {
            tid.pop_back();
            tid.push_back(((symbol_typedef*)sym.get())->type_id_);
        } else {
            o->object_type_.user_defined_type = sym;
        }
    } else {
        tid.pop_back();
        auto tid_part = tid.push_back<type_id_part_object>();
        tid_part->object_type_ = dtype;
        tid_part->decl_specifiers = dspec;
    }

    // Now look for function parameters
    {
        for (int i = 0; i < tid.size(); ++i) {
            type_id_part* pt = tid[i].get();
            if (pt->cast_to<type_id_part_function>()) {
                auto fn = pt->cast_to<type_id_part_function>();
                for (int i = 0; i < fn->parameters.size(); ++i) {
                    auto& param = fn->parameters[i];
                    resolve_placeholders(ps, param.decl_type_);
                }
            }
        }
    }

    return true;
}
std::shared_ptr<symbol> eat_class_specifier(parse_state& ps) {
    ps.push_rewind_point();

    std::shared_ptr<symbol> sym;
    if (!eat_class_head(ps, sym)) {
        ps.pop_rewind_point();
        return 0;
    }
    token tok = ps.next_token();
    if (tok.str != "{") {
        ps.rewind_();
        return 0;
    }
    
    assert(sym);
    if (sym->is_defined()) {
        throw parse_exception("Class already defined", tok);
    }
    auto class_scope = ps.enter_scope(scope_class_pass_one);
    class_scope->set_owner(sym);

    eat_member_specification(ps);

    tok = ps.next_token();
    if (tok.str != "}") {
        throw parse_exception("Expected a '}'", tok);
    }
    sym->nested_symbol_table = class_scope;
    sym->set_defined(true);
    class_scope->set_type(scope_class);

    // Resolve placeholders
    for (auto& kv : class_scope->symbols) {
        auto& vec = kv.second;
        if ((vec.mask & (LOOKUP_FLAG_OBJECT | LOOKUP_FLAG_FUNCTION | LOOKUP_FLAG_TYPEDEF)) == 0) {
            continue;
        }

        switch (sym->get_type_enum()) {
        case e_symbol_object:
            resolve_placeholders(ps, dynamic_cast<symbol_object*>(sym.get())->type_id_);
            break;
        case e_symbol_function: {
                symbol_function* sym_func = dynamic_cast<symbol_function*>(sym.get());
                for (int i = 0; i < sym_func->overloads.size(); ++i) {
                    auto& overload = sym_func->overloads[i];
                    resolve_placeholders(ps, overload->type_id_);
                }
                break;
            }
        case e_symbol_alias:
            resolve_placeholders(ps, dynamic_cast<symbol_typedef*>(sym.get())->type_id_);
            break;
        }
    }
    /*
    for (auto& it : class_scope->objects) {
        symbol_object* sym = (symbol_object*)it.second.get();
        resolve_placeholders(ps, sym->type_id_);
    }
    for (auto& it : class_scope->functions) {
        symbol_function* sym = (symbol_function*)it.second.get();
        for (int i = 0; i < sym->overloads.size(); ++i) {
            auto& overload = sym->overloads[i];
            resolve_placeholders(ps, overload->type_id_);
        }
    }
    for (auto& it : class_scope->typedefs) {
        symbol_typedef* sym = (symbol_typedef*)it.second.get();
        resolve_placeholders(ps, sym->type_id_);
    }*/

    ps.exit_scope();
    ps.pop_rewind_point();
    return sym;
}

std::shared_ptr<symbol> eat_class_specifier_limited(parse_state& ps, attribute_specifier& attr_spec) {
    ps.push_rewind_point();

    std::shared_ptr<symbol> sym;
    if (!eat_class_head(ps, sym)) {
        ps.pop_rewind_point();
        return 0;
    }
    sym->attrib_spec.merge(attr_spec);

    if (!accept(ps, "{")) {
        ps.rewind_();
        return 0;
    }
    
    assert(sym);
    if (sym->is_defined()) {
        throw parse_exception("Class already defined", ps.latest_token);
    }
    auto class_scope = ps.enter_scope(scope_class_pass_one);
    class_scope->set_owner(sym);

    eat_member_specification_limited(ps);

    expect(ps, "}");

    sym->nested_symbol_table = class_scope;
    sym->set_defined(true);
    class_scope->set_type(scope_class);

    // Resolve placeholders
    for (auto& kv : class_scope->symbols) {
        auto& vec = kv.second;
        if ((vec.mask & (LOOKUP_FLAG_OBJECT | LOOKUP_FLAG_FUNCTION | LOOKUP_FLAG_TYPEDEF)) == 0) {
            continue;
        }

        switch (sym->get_type_enum()) {
        case e_symbol_object:
            resolve_placeholders(ps, dynamic_cast<symbol_object*>(sym.get())->type_id_);
            break;
        case e_symbol_function: {
                symbol_function* sym_func = dynamic_cast<symbol_function*>(sym.get());
                for (int i = 0; i < sym_func->overloads.size(); ++i) {
                    auto& overload = sym_func->overloads[i];
                    resolve_placeholders(ps, overload->type_id_);
                }
                break;
            }
        case e_symbol_alias:
            resolve_placeholders(ps, dynamic_cast<symbol_typedef*>(sym.get())->type_id_);
            break;
        }
    }
    /*for (auto& it : class_scope->objects) {
        symbol_object* sym = (symbol_object*)it.second.get();
        resolve_placeholders(ps, sym->type_id_);
    }
    for (auto& it : class_scope->functions) {
        symbol_function* sym = (symbol_function*)it.second.get();
        for (int i = 0; i < sym->overloads.size(); ++i) {
            auto& overload = sym->overloads[i];
            resolve_placeholders(ps, overload->type_id_);
        }
    }
    for (auto& it : class_scope->typedefs) {
        symbol_typedef* sym = (symbol_typedef*)it.second.get();
        resolve_placeholders(ps, sym->type_id_);
    }*/

    ps.exit_scope();
    ps.pop_rewind_point();
    return sym;
}