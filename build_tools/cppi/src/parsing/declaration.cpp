#include "declaration.hpp"

#include "parsing/attribute_specifier.hpp"
#include "parsing/balanced_token.hpp"
#include "parsing/common.hpp"
#include "parsing/function_definition.hpp"
#include "parsing/class_specifier.hpp"
#include "parsing/decl_specifier.hpp"
#include "parsing/namespace.hpp"
#include "parsing/enum_specifier.hpp"
#include "parsing/type_specifier.hpp"


bool eat_declaration(parse_state& ps, bool ignore_free_standing_attribs) {
    // TODO: is eat_simple_declaration or function definition now
    // TODO: parse other structures too

    ps.push_rewind_point();
    if (!ignore_free_standing_attribs) {
        if (eat_attribute_specifier_seq(ps)) {
            // init-declarator-list is required if attributes are present here, but there's no harm in ignoring this rule
        }
    }

    if (eat_namespace_definition(ps, false)) {
        return true;
    }

    decl_spec dspec;
    decl_type dtype;
    if (!eat_decl_specifier_seq(ps, dspec, dtype)) {
        // Not a simple declaration or function definition
        ps.rewind_(); // need to rewind in case attribute-specifier-seq was eaten
        return false;
    }
    std::vector<declarator> decl_list;
    if (dspec.has_decl_flag(DECL_TYPEDEF)) {
        if (!eat_declarator_list(ps, dspec, dtype, &decl_list)) {
            throw parse_exception("Expected a declarator", ps.next_token());
        }
    } else {
        eat_init_declarator_list(ps, dspec, dtype, &decl_list);
        if (decl_list.size() == 1 && decl_list[0].is_func()) {
            // Try function body
            if (eat_function_body(ps, &decl_list[0])) {
                ps.pop_rewind_point();
                return true;
            }
        } 
    }

    token tok;
    if (!eat_token(ps, ";", &tok)) {
        throw parse_exception("Expected a ;", tok);
        return false;
    }

    ps.pop_rewind_point();
    return true;
}


bool eat_type_id(parse_state& ps, type_id& tid) {
    decl_spec dspec;
    decl_type dtype;
    if (!eat_type_specifier_seq(ps, dspec, dtype)) {
        return false;
    }
    declarator d;
    eat_declarator(ps, dspec, dtype, &d, 0, true);

    tid = d.decl_type_;

    return true;
}