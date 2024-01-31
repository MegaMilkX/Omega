#pragma once

#include "parse_state.hpp"
#include "decl_specifier.hpp"

/*
struct type_specifier {

};
struct decl_specifier {

};*/
/*
struct type_specifier_seq {

};
struct decl_or_type_specifier_seq {

};*/

bool eat_type_name(
    parse_state& ps,
    std::shared_ptr<symbol>& sym,
    const std::shared_ptr<symbol_table>& specific_scope = 0,
    bool as_placeholder = false
);

bool eat_decltype_specifier(parse_state& ps);
bool eat_class_or_decltype(parse_state& ps);
bool eat_trailing_type_specifier(parse_state& ps, decl_spec& dspec, decl_type& dtype, bool as_placeholder = false);
bool eat_trailing_type_specifier_seq(parse_state& ps, decl_spec& dspec, decl_type& dtype);
bool eat_type_specifier(parse_state& ps, decl_spec& dspec, decl_type& dtype);
bool eat_type_specifier_seq(parse_state& ps, decl_spec& dspec, decl_type& dtype);

bool eat_simple_type_specifier(parse_state& ps, decl_type* sts, bool as_placeholder = false);
bool eat_elaborated_type_specifier(parse_state& ps, bool as_placeholder = false);
bool eat_typename_specifier(parse_state& ps, bool as_placeholder = false);

bool eat_template_argument_list(parse_state& ps);
bool eat_simple_template_id(
    parse_state& ps, 
    std::shared_ptr<symbol>& sym, 
    const std::shared_ptr<symbol_table>& specific_scope = 0,
    bool as_placeholder = false
);

bool eat_cv_qualifier(parse_state& ps, int* out);
bool eat_cv_qualifier_seq(parse_state& ps, int* cv_flags);

inline std::shared_ptr<symbol> eat_class_name(parse_state& ps, const std::shared_ptr<symbol_table>& specific_scope = 0) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_identifier) {
        ps.rewind_();
        return 0;
    }
    std::shared_ptr<symbol> sym;
    if (specific_scope) {
        sym = specific_scope->get_symbol(tok.str, LOOKUP_FLAG_CLASS);
    } else {
        sym = ps.get_current_scope()->lookup(tok.str, LOOKUP_FLAG_CLASS);
    }
    if (sym && sym->is<symbol_class>()) {
        ps.pop_rewind_point();
        return sym;
    }

    ps.rewind_();

    // Not a simple class, try template
    if (eat_simple_template_id(ps, sym, specific_scope, false)) {
        return sym;
    }

    return 0;
}
inline std::shared_ptr<symbol> eat_enum_name(parse_state& ps, const std::shared_ptr<symbol_table>& specific_scope = 0) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_identifier) {
        ps.rewind_();
        return 0;
    }
    std::shared_ptr<symbol> sym;
    if (specific_scope) {
        sym = specific_scope->get_symbol(tok.str, LOOKUP_FLAG_ENUM);
    } else {
        sym = ps.get_current_scope()->lookup(tok.str, LOOKUP_FLAG_ENUM);
    }
    if (!sym || !sym->is<symbol_enum>()) {
        ps.rewind_();
        return 0;
    }

    ps.pop_rewind_point();
    return sym;
}
inline std::shared_ptr<symbol> eat_typedef_name(parse_state& ps, const std::shared_ptr<symbol_table>& specific_scope = 0) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_identifier) {
        ps.rewind_();
        return 0;
    }
    std::shared_ptr<symbol> sym;
    if (specific_scope) {
        sym = specific_scope->get_symbol(tok.str, LOOKUP_FLAG_TYPEDEF);
    } else {
        sym = ps.get_current_scope()->lookup(tok.str, LOOKUP_FLAG_TYPEDEF);
    }
    if (!sym || !sym->is<symbol_typedef>()) {
        ps.rewind_();
        return 0;
    }

    ps.pop_rewind_point();
    return sym;
}
inline std::shared_ptr<symbol> eat_template_name(parse_state& ps, const std::shared_ptr<symbol_table>& specific_scope = 0) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_identifier) {
        ps.rewind_();
        return false;
    }
    
    std::shared_ptr<symbol> sym;
    if (specific_scope) {
        sym = specific_scope->get_symbol(tok.str, LOOKUP_FLAG_TEMPLATE);
    } else {
        sym = ps.get_current_scope()->lookup(tok.str, LOOKUP_FLAG_TEMPLATE);
    }
    if (!sym || !sym->is<symbol_template>()) {
        ps.rewind_();
        return 0;
    }

    ps.pop_rewind_point();
    return sym;
}
