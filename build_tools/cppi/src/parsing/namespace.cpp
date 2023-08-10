#include "namespace.hpp"

#include "parsing/common.hpp"

#include "attribute_specifier.hpp"
#include "class_specifier.hpp"
#include "enum_specifier.hpp"
#include "balanced_token.hpp"
#include "declaration.hpp"
#include "template_declaration.hpp"


bool eat_namespace_body(parse_state& ps) {
    while (true) {
        if (eat_declaration(ps)) {
            continue;
        }
        if (eat_template_declaration(ps)) {
            continue;
        }
        break;
    }
    return true;
}
bool eat_namespace_body_limited(parse_state& ps) {
    while (true) {
        attribute_specifier attr_spec;
        if (eat_namespace_definition(ps, true)) {
            continue;
        } else if (eat_attribute_specifier_seq(ps, attr_spec)) {
            if (attr_spec.find_attrib("cppi_class")) {
                expect(ps, ";");
                if (!eat_class_specifier_limited(ps, attr_spec)) {
                    throw parse_exception("cppi_class attribute must be followed by a class definition", ps.latest_token);
                }
                continue;
            } else if(attr_spec.find_attrib("cppi_enum")) {
                expect(ps, ";");
                if (!eat_enum_specifier_limited(ps)) {
                    throw parse_exception("cppi_enum attribute must be followed by an enumeration definition", ps.latest_token);
                }
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
bool eat_namespace_definition(parse_state& ps, bool limited) {
    ps.push_rewind_point();

    bool is_inline = false;
    if (accept(ps, kw_inline)) {
        is_inline = true;
    }

    if (!accept(ps, kw_namespace)) {
        ps.rewind_();
        return false;
    }

    std::string namespace_name;
    token tok;
    if (eat_token(ps, tt_identifier, &tok)) {
        namespace_name = tok.str;
    }

    std::shared_ptr<symbol> sym = ps.get_current_scope()->lookup(tok.str, LOOKUP_FLAG_NAMESPACE);
    std::shared_ptr<symbol_table> scope;
    if (sym) {
        scope = sym->nested_symbol_table;
        ps.enter_scope(scope);
    } else {
        sym.reset(new symbol_namespace);
        sym->name = tok.str;
        sym->file = tok.file;
        ps.add_symbol(sym);
        symbol_namespace* sym_namespace = (symbol_namespace*)sym.get();
        sym_namespace->is_inline = is_inline;

        auto parent_scope = ps.get_current_scope();
        scope = ps.enter_scope(scope_namespace);
        scope->set_owner(sym);
        scope->set_type(scope_namespace);
        scope->set_enclosing(parent_scope);
        sym->nested_symbol_table = scope;
        sym->defined = true;
    }

    expect(ps, "{");

    if (limited) {
        eat_namespace_body_limited(ps);
    } else {
        eat_namespace_body(ps);
    }

    expect(ps, "}");
    ps.exit_scope();
    ps.pop_rewind_point();
    return true;
}
