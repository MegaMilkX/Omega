#include "primary_expression.hpp"

#include "common.hpp"
#include "ast/ast.hpp"


bool eat_nested_name_specifier(
    parse_state& ps, 
    std::shared_ptr<symbol_table>& scope, 
    bool as_placeholder
) {
    // TODO: review nested-name-specifier

    if(!as_placeholder) {
        ps.push_rewind_point();
        token tok;

        scope = ps.get_current_scope();
        if (accept(ps, "::")) {
            scope = ps.get_root_scope();
        } else if(eat_token(ps, tt_identifier, &tok)) {
            token tok_scope_name = tok;
            std::string scope_name = tok.str;
            if (!eat_token(ps, "::", &tok)) {
                ps.rewind_();
                scope = 0;
                return false;
            }
            auto symbol = scope->lookup(
                scope_name,
                LOOKUP_FLAG_CLASS | LOOKUP_FLAG_NAMESPACE | LOOKUP_FLAG_ENUM
            );
            printf("eat_nested_name_specifier: %s\n", symbol->name.c_str());
            if (!symbol) {
                throw parse_exception("Identifier followed by :: is not a known symbol", tok_scope_name);
            }
            if (!symbol->nested_symbol_table) {
                throw parse_exception("Identifier followed by :: does not have a nested scope", tok_scope_name);
            }
            scope = symbol->nested_symbol_table;
        } else {
            ps.rewind_();
            scope = 0;
            return false;
        }
        ps.pop_rewind_point();

        // TODO: decltype_specifier
        while (true) {
            ps.push_rewind_point();
            token tok;
            if (eat_token(ps, tt_identifier, &tok)) {
                token tok_scope_name = tok;
                std::string scope_name = tok.str;
                if (eat_token(ps, "::", &tok)) {
                    auto symbol = scope->get_symbol(
                        scope_name,
                        LOOKUP_FLAG_CLASS | LOOKUP_FLAG_NAMESPACE | LOOKUP_FLAG_ENUM
                    );
                    printf("eat_nested_name_specifier: %s\n", symbol->name.c_str());
                    if (!symbol) {
                        throw parse_exception("Identifier followed by :: is not a known symbol", tok_scope_name);
                    }
                    if (!symbol->nested_symbol_table) {
                        throw parse_exception("Identifier followed by :: does not have a nested scope", tok_scope_name);
                    }
                    scope = symbol->nested_symbol_table;
                    ps.pop_rewind_point();
                    continue;
                } else {
                    ps.rewind_();
                    return true;
                }
            } else {
                ps.pop_rewind_point();
                return true;
            }// TODO: template_opt simple-template-id
        }
    } else {
        ps.push_rewind_point();
        token tok;

        scope = 0;
        if (accept(ps, "::")) {
            //
        } else if(eat_token(ps, tt_identifier, &tok)) {
            if (!eat_token(ps, "::", &tok)) {
                ps.rewind_();
                return false;
            }
        } else {
            ps.rewind_();
            return false;
        }
        ps.pop_rewind_point();

        // TODO: decltype_specifier
        while (true) {
            ps.push_rewind_point();
            token tok;
            if (eat_token(ps, tt_identifier, &tok)) {
                if (eat_token(ps, "::", &tok)) {
                    ps.pop_rewind_point();
                    continue;
                } else {
                    ps.rewind_();
                    return true;
                }
            } else {
                ps.pop_rewind_point();
                return true;
            }// TODO: template_opt simple-template-id
        }
    }
}

bool eat_qualified_id(parse_state& ps) {
    std::shared_ptr<symbol_table> scope;
    if (!eat_nested_name_specifier(ps, scope)) {
        return false;
    }
    eat_keyword(ps, kw_template);
    if (!eat_unqualified_id(ps)) {
        throw parse_exception("Expected unqualified_id", ps.next_token());
    }
    return true;
}
bool eat_unqualified_id(parse_state& ps) {
    token tok;
    if (eat_token(ps, tt_identifier, &tok)) {
        return true;
    }/*
    if (eat_operator_function_id(ps)) {
        return true;
    }
    if (eat_conversion_function_id(ps)) {
        return true;
    }
    if (eat_literal_operator_id(ps)) {
        return true;
    }*//*
    if (accept(ps, "~")) {
        if (eat_class_name(ps)) {
            return true;
        }
        if (eat_decltype_specifier(ps)) {
            return true;
        }
    }*//*
    if (eat_template_id(ps)) {
        return true;
    }*/
    return false;
}
bool eat_id_expression(parse_state& ps) {
    if (eat_unqualified_id(ps)) {
        return true;
    }
    if (eat_qualified_id(ps)) {
        return true;
    }
    return false;
}
bool eat_primary_expression(parse_state& ps) {
    token tok;
    if (eat_token(ps, tt_literal, &tok)) {
        return true;
    }
    if (eat_token(ps, tt_string_literal, &tok)) {
        return true;
    }
    if (eat_keyword(ps, kw_this)) {
        return true;
    }
    if (accept(ps, "(")) {
        // TODO: expression
        eat_balanced_token_seq(ps);
        expect(ps, ")");
        return true;
    }
    if (eat_id_expression(ps)) {
        return true;
    }/*
    if (eat_lambda_expression(ps)) {
        return true;
    }*/
    return false;
}
