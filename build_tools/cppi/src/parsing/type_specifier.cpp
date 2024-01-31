#include "type_specifier.hpp"
#include "attribute_specifier.hpp"
#include "enum_specifier.hpp"
#include "class_specifier.hpp"
#include "common.hpp"
#include "balanced_token.hpp"
#include "decl_specifier.hpp"
#include "primary_expression.hpp"
#include "parse_exception.hpp"
#include "declaration.hpp"


bool eat_type_name_placeholder(parse_state& ps) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_identifier) {
        ps.rewind_();
        return false;
    }

    if (eat_token(ps, "<", &tok)) {
        eat_template_argument_list(ps);

        if (!eat_token(ps, ">", &tok)) {
            if (!eat_token(ps, ">>", &tok)) {
                throw parse_exception("Expected a '>'", tok);
            }
            ps.get_latest_token().str = ">";

            token tk = tok;
            tk.str = ">";
            ps.push_token(tk);
        }
    }

    ps.pop_rewind_point();
    return true;
}
bool eat_simple_template_id_placeholder(parse_state& ps) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_identifier) {
        ps.rewind_();
        return false;
    }

    if (!eat_token(ps, "<", &tok)) {
        return false;
    }

    eat_template_argument_list(ps);

    if (!eat_token(ps, ">", &tok)) {
        if (!eat_token(ps, ">>", &tok)) {
            throw parse_exception("Expected a '>'", tok);
        }
        ps.get_latest_token().str = ">";

        token tk = tok;
        tk.str = ">";
        ps.push_token(tk);
    }

    ps.pop_rewind_point();
    return true;
}
bool eat_type_name(
    parse_state& ps,
    std::shared_ptr<symbol>& sym,
    const std::shared_ptr<symbol_table>& specific_scope,
    bool as_placeholder
) {
    if (as_placeholder) {
        return eat_type_name_placeholder(ps);
    } else {
        if (eat_simple_template_id(ps, sym, specific_scope)) {
            return true;
        }
        if (sym = eat_class_name(ps, specific_scope)) {
            return true;
        }
        if (sym = eat_enum_name(ps, specific_scope)) {
            return true;
        }
        if (sym = eat_typedef_name(ps, specific_scope)) {
            return true;
        }
        return false;
    }
}

bool eat_decltype_specifier(parse_state& ps) {
    if (!eat_keyword(ps, kw_decltype)) {
        return false;
    }
    throw parse_exception("decltype not implemented", ps.next_token());

    token tok;
    if (!eat_token(ps, "(", &tok)) {
        throw parse_exception("Expected an '('", tok);
    }

    // TODO: expression
    // TODO: auto
    if (!eat_balanced_token_seq(ps)) {
        throw parse_exception("Expected a balanced_token_seq", ps.next_token());
    }

    if (!eat_token(ps, ")", &tok)) {
        throw parse_exception("Expected a ')'", tok);
    }
    return true;
}
bool eat_class_or_decltype(parse_state& ps) {
    std::shared_ptr<symbol_table> scope;
    if (eat_nested_name_specifier(ps, scope)) {
        if (!eat_class_name(ps)) {
            throw parse_exception("Expected a class_name", ps.next_token());
        }
        return true;
    }
    if (eat_class_name(ps)) {
        return true;
    }
    if (eat_decltype_specifier(ps)) {
        return true;
    }
    return false;
}

bool eat_simple_type_specifier(parse_state& ps, decl_type* sts, bool as_placeholder) {
    if(sts->type_flags == 0) {
        std::shared_ptr<symbol> sym;
        std::shared_ptr<symbol_table> scope;
        if (eat_nested_name_specifier(ps, scope, as_placeholder)) {
            if (eat_keyword(ps, kw_template)) {
                if (!eat_simple_template_id(ps, sym, scope, as_placeholder)) {
                    throw parse_exception("Expected a simple-template-id", ps.next_token());
                }
                sts->compatibility_flags = 0;
                sts->type_flags |= fl_user_defined;
                sts->user_defined_type = sym;
                return true;
            } else {
                if (!eat_type_name(ps, sym, scope, as_placeholder)) {
                    throw parse_exception("Expected a type-name", ps.next_token());
                }
                sts->compatibility_flags = 0;
                sts->type_flags |= fl_user_defined;
                sts->user_defined_type = sym;
                return true;
            }
            return false;
        }
        if (eat_type_name(ps, sym, 0, as_placeholder)) {
            sts->compatibility_flags = 0;
            sts->type_flags |= fl_user_defined;
            sts->user_defined_type = sym;
            return true;
        }
    }
    if ((sts->compatibility_flags & fl_char) && eat_keyword(ps, kw_char)) {
        sts->compatibility_flags &= compat_char;
        sts->type_flags |= fl_char;
        return true;
    }
    if ((sts->compatibility_flags & fl_char16_t) && eat_keyword(ps, kw_char16_t)) {
        sts->compatibility_flags &= compat_char16_t;
        sts->type_flags |= fl_char16_t;
        return true;
    }
    if ((sts->compatibility_flags & fl_char32_t) && eat_keyword(ps, kw_char32_t)) {
        sts->compatibility_flags &= compat_char32_t;
        sts->type_flags |= fl_char32_t;
        return true;
    }
    if ((sts->compatibility_flags & fl_wchar_t) && eat_keyword(ps, kw_wchar_t)) {
        sts->compatibility_flags &= compat_wchar_t;
        sts->type_flags |= fl_wchar_t;
        return true;
    }
    if ((sts->compatibility_flags & fl_bool) && eat_keyword(ps, kw_bool)) {
        sts->compatibility_flags &= compat_bool;
        sts->type_flags |= fl_bool;
        return true;
    }
    if ((sts->compatibility_flags & fl_short) && eat_keyword(ps, kw_short)) {
        sts->compatibility_flags &= compat_short;
        sts->type_flags |= fl_short;
        return true;
    }
    if ((sts->compatibility_flags & fl_int) && eat_keyword(ps, kw_int)) {
        sts->compatibility_flags &= compat_int;
        sts->type_flags |= fl_int;
        return true;
    }
    if ((sts->compatibility_flags & fl_long) && eat_keyword(ps, kw_long)) {
        if ((sts->type_flags & fl_long) == 0) {
            sts->compatibility_flags &= compat_long;
            sts->type_flags |= fl_long;
        } else {
            sts->compatibility_flags &= compat_longlong;
            sts->type_flags &= ~fl_long;
            sts->type_flags |= fl_longlong;
        }
        return true;
    }
    if ((sts->compatibility_flags & fl_signed) && eat_keyword(ps, kw_signed)) {
        sts->compatibility_flags &= compat_signed;
        sts->type_flags |= fl_signed;
        return true;
    }
    if ((sts->compatibility_flags & fl_unsigned) && eat_keyword(ps, kw_unsigned)) {
        sts->compatibility_flags &= compat_unsigned;
        sts->type_flags |= fl_unsigned;
        return true;
    }
    if ((sts->compatibility_flags & fl_float) && eat_keyword(ps, kw_float)) {
        sts->compatibility_flags &= compat_float;
        sts->type_flags |= fl_float;
        return true;
    }
    if ((sts->compatibility_flags & fl_double) && eat_keyword(ps, kw_double)) {
        sts->compatibility_flags &= compat_double;
        sts->type_flags |= fl_double;
        return true;
    }
    if ((sts->compatibility_flags & fl_void) && eat_keyword(ps, kw_void)) {
        sts->compatibility_flags &= compat_void;
        sts->type_flags |= fl_void;
        return true;
    }
    if ((sts->compatibility_flags & fl_auto) && eat_keyword(ps, kw_auto)) {
        sts->compatibility_flags &= compat_auto;
        sts->type_flags |= fl_auto;
        return true;
    }
    if (eat_decltype_specifier(ps)) {
        return true;
    }
    return false;
}

bool eat_cv_qualifier(parse_state& ps, int* out) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_keyword) {
        ps.rewind_();
        return false;
    }
    if (tok.kw_type == kw_const) {
        ps.pop_rewind_point();
        if(out) *out = DECL_CV_CONST;
        return true;
    }
    if (tok.kw_type == kw_volatile) {
        ps.pop_rewind_point();
        if(out) *out = DECL_CV_VOLATILE;
        return true;
    }
    ps.rewind_();
    return false;
}
bool eat_cv_qualifier_seq(parse_state& ps, int* cv_flags) {
    if (!eat_cv_qualifier(ps, cv_flags)) {
        return false;
    }
    while (eat_cv_qualifier(ps, cv_flags)) {

    }
    return true;
}

bool eat_elaborated_type_specifier(parse_state& ps, bool as_placeholder) {
    e_class_key key;
    if (eat_class_key(ps, key)) {
        std::shared_ptr<symbol_table> scope;
        if (eat_nested_name_specifier(ps, scope, as_placeholder)) {
            std::shared_ptr<symbol> sym;
            if (eat_keyword(ps, kw_template)) {
                if (!eat_simple_template_id(ps, sym, scope, as_placeholder)) {
                    throw parse_exception("Expected a simple_template_id", ps.next_token());
                }
                return true;
            }
            if (eat_simple_template_id(ps, sym, scope, as_placeholder)) {
                return true;
            }
            // TODO:
            if (eat_identifier(ps)) {
                return true;
            }
            throw parse_exception("Expected an identifier", ps.next_token());
        }
        eat_attribute_specifier_seq(ps);
        eat_nested_name_specifier(ps, scope, as_placeholder);
        if (!eat_identifier(ps)) {
            throw parse_exception("Expected an identifier", ps.next_token());
        }
        return true;
    } else if(eat_keyword(ps, kw_enum)) {
        std::shared_ptr<symbol_table> scope;
        eat_nested_name_specifier(ps, scope, as_placeholder);
        if (!eat_identifier(ps)) {
            throw parse_exception("Expected an identifier", ps.next_token());
        }
        return true;
    }
    return false;
}
bool eat_typename_specifier(parse_state& ps, bool as_placeholder) {
    if (!eat_keyword(ps, kw_typename)) {
        return false;
    }
    std::shared_ptr<symbol_table> scope;
    if (!eat_nested_name_specifier(ps, scope, as_placeholder)) {
        throw parse_exception("Expected a nested_name_specifier", ps.next_token());
    }
    if (eat_keyword(ps, kw_template)) {
        std::shared_ptr<symbol> sym;
        if (!eat_simple_template_id(ps, sym, 0, as_placeholder)) {
            throw parse_exception("Expected a simple_template_id", ps.next_token());
        }
        return true;
    }
    std::shared_ptr<symbol> sym;
    if (eat_simple_template_id(ps, sym, 0, as_placeholder)) {
        return true;
    }
    if (eat_identifier(ps)) {
        return true;
    }
    throw parse_exception("Expected an identifier", ps.next_token());
}
bool eat_trailing_type_specifier(parse_state& ps, decl_spec& dspec, decl_type& dtype, bool as_placeholder) {
    ps.push_rewind_point();
    if (eat_simple_type_specifier(ps, &dtype, as_placeholder)) {
        if (as_placeholder) {
            std::shared_ptr<symbol_placeholder> sptr(new symbol_placeholder);
            dtype.user_defined_type = sptr;
            ps.pop_rewind_get_tokens(sptr->token_cache);
        } else {
            ps.pop_rewind_point();
        }
        return true;
    }
    if (eat_elaborated_type_specifier(ps, as_placeholder)) {
        if (as_placeholder) {
            std::shared_ptr<symbol_placeholder> sptr(new symbol_placeholder);
            dtype.user_defined_type = sptr;
            ps.pop_rewind_get_tokens(sptr->token_cache);
        } else {
            ps.pop_rewind_point();
        }
        return true;
    }
    if (eat_typename_specifier(ps, as_placeholder)) {
        if (as_placeholder) {
            std::shared_ptr<symbol_placeholder> sptr(new symbol_placeholder);
            dtype.user_defined_type = sptr;
            ps.pop_rewind_get_tokens(sptr->token_cache);
        } else {
            ps.pop_rewind_point();
        }
        return true;
    }
    int cv_flag = 0;
    if (eat_cv_qualifier(ps, &cv_flag)) {
        if (!dspec.is_decl_compatible(cv_flag)) {
            // TODO: Now reporting next token after the specifier, fix this
            throw parse_exception("Incompatible cv specifier", ps.next_token());
        }
        dspec.set_decl_flag(cv_flag, ~0);
        ps.pop_rewind_point();
        return true;
    }
    ps.rewind_();
    return false;
}
bool eat_trailing_type_specifier_seq(parse_state& ps, decl_spec& dspec, decl_type& dtype) {
    bool as_placeholder = ps.get_current_scope()->get_type() == scope_class_pass_one;

    if (!eat_trailing_type_specifier(ps, dspec, dtype, as_placeholder)) {
        return false;
    }
    while (eat_trailing_type_specifier(ps, dspec, dtype, as_placeholder)) {

    }
    eat_attribute_specifier_seq(ps);
    return true;
}
bool eat_type_specifier(parse_state& ps, decl_spec& dspec, decl_type& dtype) {
    std::shared_ptr<symbol> type_sym;
    if (type_sym = eat_class_specifier(ps)) {
        if ((dtype.compatibility_flags & fl_user_defined) == 0) {
            throw parse_exception("class-specifier is not valid here", ps.next_token());
        }
        dtype.type_flags |= fl_user_defined;
        dtype.compatibility_flags &= compat_custom;
        dtype.user_defined_type = type_sym;
        return true;
    }
    if (eat_enum_specifier(ps)) {
        if ((dtype.compatibility_flags & fl_user_defined) == 0) {
            throw parse_exception("enum-specifier is not valid here", ps.next_token());
        }
        dtype.type_flags |= fl_user_defined;
        dtype.compatibility_flags &= compat_custom;
        return true;
    }
    if (ps.get_current_scope()->is_parsing_class_first_pass()) {
        return eat_trailing_type_specifier(ps, dspec, dtype, true);
    } else {
        return eat_trailing_type_specifier(ps, dspec, dtype);
    }
    return false;
}
bool eat_type_specifier_seq(parse_state& ps, decl_spec& dspec, decl_type& dtype) {
    if (!eat_type_specifier(ps, dspec, dtype)) {
        return false;
    }
    while (eat_type_specifier(ps, dspec, dtype)) {

    }
    eat_attribute_specifier_seq(ps);
    return true;
}

bool eat_template_argument(parse_state& ps) {
    type_id tid;
    if (eat_type_id(ps, tid)) {
        return true;
    }
    /*
    if (eat_constant_expression(ps)) {
        return true;
    }
    if (eat_id_expression(ps)) {
        return true;
    }*/
    return false;
}
bool eat_template_argument_list(parse_state& ps) {
    if (!eat_template_argument(ps)) {
        return false;
    }
    token tok;
    if (eat_token(ps, "...", &tok)) {
        return true;
    }
    while (true) {
        if (!eat_token(ps, ",", &tok)) {
            return true;
        }
        if (!eat_template_argument(ps)) {
            throw parse_exception("Expected a template-argument", ps.next_token());
        }
        if (eat_token(ps, "...", &tok)) {
            return true;
        }
    }
}
bool eat_simple_template_id(
    parse_state& ps,
    std::shared_ptr<symbol>& sym,
    const std::shared_ptr<symbol_table>& specific_scope,
    bool as_placeholder
) {
    // TODO:
    if (as_placeholder) {
        return eat_simple_template_id_placeholder(ps);
    }

    if (!(sym = eat_template_name(ps, specific_scope))) {
        
        return false;
    }
    token tok;
    if (!eat_token(ps, "<", &tok)) {
        throw parse_exception("Expected a template-argument-list", tok);
    }

    eat_template_argument_list(ps);

    if (!eat_token(ps, ">", &tok)) {
        if (!eat_token(ps, ">>", &tok)) {
            throw parse_exception("Expected a '>'", tok);
        }
        ps.get_latest_token().str = ">";

        token tk = tok;
        tk.str = ">";
        ps.push_token(tk);
        return true;
    }
    return true;
}
