#include "enum_specifier.hpp"
#include "balanced_token.hpp"
#include "attribute_specifier.hpp"
#include "type_specifier.hpp"
#include "common.hpp"
#include "primary_expression.hpp"
#include "parse_exception.hpp"


bool eat_enum_key(parse_state& ps, e_enum_key& key) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_keyword || tok.kw_type != kw_enum) {
        ps.rewind_();
        return false;
    }
    ps.pop_rewind_point();
    key = e_enum_key::ck_enum;

    ps.push_rewind_point();
    tok = ps.next_token();
    if (tok.type == tt_keyword) {
        if (tok.kw_type == kw_class) {
            key = e_enum_key::ck_enum_class;
        } else if(tok.kw_type == kw_struct) {
            key = e_enum_key::ck_enum_struct;
        }
        ps.pop_rewind_point();
        return true;
    } else {
        ps.rewind_();
        return true;
    }
}

bool eat_enum_base(parse_state& ps) {
    token tok;
    if (!eat_token(ps, ":", &tok)) {
        return false;
    }
    decl_spec dspec;
    decl_type dtype;
    if (!eat_type_specifier_seq(ps, dspec, dtype)) {
        throw parse_exception("Expected type-specifier-seq", ps.next_token());
    }
    return true;
}

bool eat_enum_head_name(parse_state& ps, std::shared_ptr<symbol>& out_sym) {
    std::shared_ptr<symbol_table> nns_scope;
    eat_nested_name_specifier(ps, nns_scope);

    token tok;
    bool has_name = eat_token(ps, tt_identifier, &tok);
    if (!has_name && nns_scope) {
        throw parse_exception("Missing enum name", tok);
    }
    if (!nns_scope) {
        nns_scope = ps.get_current_scope();
    }

    std::string enum_name;
    std::shared_ptr<symbol> sym;
    if (has_name) {
        enum_name = tok.str;

        sym = nns_scope->get_symbol(enum_name, LOOKUP_FLAG_ENUM | LOOKUP_FLAG_CLASS);
        if (sym && !sym->is<symbol_enum>()) {
            throw parse_exception("Name already declared as a different entity", tok);
        }
        if (sym) {
            out_sym = sym;
        } else {
            out_sym = std::shared_ptr<symbol>(new symbol_enum);
            out_sym->name = enum_name;
            ps.add_symbol(out_sym);
        }
        out_sym->file = tok.file;
    } else {
        out_sym = std::shared_ptr<symbol>(new symbol_enum);
        ps.add_symbol(out_sym);
    }

    return true;
}

bool eat_enum_head(parse_state& ps, std::shared_ptr<symbol>& out_sym) {
    e_enum_key key;
    if (!eat_enum_key(ps, key)) {
        return false;
    }
    eat_attribute_specifier_seq(ps);

    eat_enum_head_name(ps, out_sym);
    eat_enum_base(ps);

    ((symbol_enum*)out_sym.get())->key = key;
    return true;
}

bool eat_enumerator(parse_state& ps, std::shared_ptr<symbol>& sym) {
    token tok;
    if (!eat_token(ps, tt_identifier, &tok)) {
        return false;
    }
    sym.reset(new symbol_enumerator);
    sym->name = tok.str;
    sym->file = tok.file;
    return true;
}
bool eat_enumerator_definition(parse_state& ps, std::string& name) {
    std::shared_ptr<symbol> sym;
    if (!eat_enumerator(ps, sym)) {
        return false;
    }
    ps.add_symbol(sym);
    name = sym->name;

    if (!accept(ps, "=")) {
        return true;
    }
    if (!eat_balanced_token_seq_except(ps, { "}", "," })) {
        throw parse_exception("Expected an expression", ps.latest_token);
    }
    return true;
}
bool eat_enumerator_list(parse_state& ps) {
    bool list_empty = true;
    std::string name;
    while (eat_enumerator_definition(ps, name)) {
        list_empty = false;

        if (!accept(ps, ",")) {
            break;
        }
    }
    return !list_empty;
}

std::shared_ptr<symbol> eat_enum_specifier(parse_state& ps) {
    ps.push_rewind_point();

    std::shared_ptr<symbol> sym;
    if (!eat_enum_head(ps, sym)) {
        ps.pop_rewind_point();
        return 0;
    }
    
    if (!accept(ps, "{")) {
        ps.rewind_();
        return 0;
    }

    assert(sym);
    if (sym->is_defined()) {
        throw parse_exception("Enum already defined", ps.latest_token);
    }
    auto enum_scope = ps.enter_scope(scope_enum);
    enum_scope->set_owner(sym);

    if (eat_enumerator_list(ps)) {
        accept(ps, ",");
    }

    expect(ps, "}");

    sym->nested_symbol_table = enum_scope;
    sym->set_defined(true);

    ps.pop_rewind_point();
    return sym;
}
std::shared_ptr<symbol> eat_enum_specifier_limited(parse_state& ps) {
    ps.push_rewind_point();
    
    std::shared_ptr<symbol> sym;
    if (!eat_enum_head(ps, sym)) {
        ps.pop_rewind_point();
        return 0;
    }

    if (!accept(ps, "{")) {
        ps.rewind_();
        return 0;
    }

    assert(sym);
    if (sym->is_defined()) {
        throw parse_exception("Enum already defined", ps.latest_token);
    }
    auto enum_scope = ps.enter_scope(scope_enum);
    enum_scope->set_owner(sym);

    if (eat_enumerator_list(ps)) {
        accept(ps, ",");
    }

    expect(ps, "}");
    ps.exit_scope();

    sym->nested_symbol_table = enum_scope;
    sym->set_defined(true);

    ps.pop_rewind_point();
    return sym;
}