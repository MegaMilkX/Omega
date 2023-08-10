#include "function_definition.hpp"
#include "attribute_specifier.hpp"
#include "decl_specifier.hpp"
#include "common.hpp"
#include "balanced_token.hpp"
#include "type_specifier.hpp"
#include "class_specifier.hpp"
#include "primary_expression.hpp"
#include "parse_exception.hpp"


bool eat_ptr_operator(parse_state& ps, ptr_decl_part* ptr_part) {
    token tok;
    std::shared_ptr<symbol_table> scope;
    if (eat_token(ps, "*", &tok)) {
        eat_attribute_specifier_seq(ps);
        int cv_flags = 0;
        eat_cv_qualifier_seq(ps, &cv_flags);
        ptr_part->type = PTR_DECL_PTR;
        ptr_part->cv_flags = cv_flags;
        return true;
    } else if(eat_token(ps, "&", &tok)) {
        eat_attribute_specifier_seq(ps);
        ptr_part->type = PTR_DECL_LVALUE_REF;
        return true;
    } else if(eat_token(ps, "&&", &tok)) {
        eat_attribute_specifier_seq(ps);
        ptr_part->type = PTR_DECL_RVALUE_REF;
        return true;
    } else if(eat_nested_name_specifier(ps, scope)) {
        // TODO: POINTER TO MEMBER
        if (!eat_token(ps, "*", &tok)) {
            throw parse_exception("Expected a '*'", tok);
        }
        eat_attribute_specifier_seq(ps);
        int cv_flags = 0;
        eat_cv_qualifier_seq(ps, &cv_flags);
        ptr_part->type = PTR_DECL_PTR;
        ptr_part->cv_flags = cv_flags;
        return true;
    } else {
        return false;
    }
}

bool eat_ref_qualifier(parse_state& ps) {
    token tok;
    if (eat_token(ps, "&", &tok)) {
        return true;
    }
    if (eat_token(ps, "&&", &tok)) {
        return true;
    }
    return false;
}

bool eat_dynamic_exception_specification(parse_state& ps) {
    if (!eat_keyword(ps, kw_throw)) {
        return false;
    }
    token tok;
    if (!eat_token(ps, "(", &tok)) {
        throw parse_exception("Expected an '('", tok);
    }

    // TODO:
    eat_balanced_token_seq(ps);

    if (!eat_token(ps, ")", &tok)) {
        throw parse_exception("Expected a ')'", tok);
    }
    return true;
}
bool eat_noexcept_specification(parse_state& ps) {
    if (!eat_keyword(ps, kw_noexcept)) {
        return false;
    }
    token tok;
    if (!eat_token(ps, "(", &tok)) {
        return true;
    }

    // TODO:
    eat_balanced_token_seq(ps);

    if (!eat_token(ps, ")", &tok)) {
        throw parse_exception("Expected a ')'", tok);
    }
    return true;
}
bool eat_exception_specification(parse_state& ps) {
    if (eat_dynamic_exception_specification(ps)) {
        return true;
    }
    if (eat_noexcept_specification(ps)) {
        return true;
    }
    return false;
}
bool eat_parameter_declaration(parse_state& ps, declarator* d) {
    // TODO: abstract-declarator
    // parameter can be a pointer with no name
    // and have an initializer at the same time
    ps.push_rewind_point();
    eat_attribute_specifier_seq(ps);
    decl_spec dspec;
    decl_type dtype;
    if (!eat_decl_specifier_seq(ps, dspec, dtype)) {
        ps.rewind_();
        return false;
    }
    eat_declarator(ps, dspec, dtype, d, 0, true);/*
    auto t = d->decl_type_.push_back<type_id_part_object>();
    t->object_type_ = dss.type_;*/

    token tok;
    if (eat_token(ps, "=", &tok)) {
        eat_balanced_token_seq_except(ps, { ",", ")" });
    }
    ps.pop_rewind_point();
    return true;
}
bool eat_parameter_declaration_clause(parse_state& ps, std::vector<declarator>* decl_list) {
    // TODO: '...' for templates
    declarator d;
    while (eat_parameter_declaration(ps, &d)) {
        decl_list->push_back(std::move(d));

        token tok;
        if (!eat_token(ps, ",", &tok)) {
            return true;
        }
        d = declarator();
    }
    return true;
}
bool eat_array_braces(parse_state& ps) {
    token tok;
    if (eat_token(ps, "[", &tok)) {
        // TODO:
        eat_balanced_token_seq(ps);

        if (!eat_token(ps, "]", &tok)) {
            throw parse_exception("Expected a ']'", tok);
        }
        return true;
    }
    return false;
}
bool eat_parameters_and_qualifiers(parse_state& ps, declarator* d) {
    token tok;
    if (!eat_token(ps, "(", &tok)) {
        return false;
    }
    auto parameter_scope = ps.enter_scope();

    if (!d->decl_type_.empty()) {
        if (d->decl_type_.get_last()->cast_to<type_id_part_function>()) {
            throw parse_exception("Function returning a function is not allowed", ps.next_token());
        } else if (d->decl_type_.get_last()->cast_to<type_id_part_array>()) {
            throw parse_exception("Array of functions is not allowed", ps.next_token());
        }
    }
    auto fn = d->decl_type_.push_back<type_id_part_function>();
    fn->parameter_scope = parameter_scope;
    

    std::vector<declarator> declarations;
    eat_parameter_declaration_clause(ps, &declarations);
    fn->parameters = std::move(declarations);

    if (!eat_token(ps, ")", &tok)) {
        throw parse_exception("Expected a ')'", tok);
    }
    ps.exit_scope();

    int cv_flags = 0;
    eat_cv_qualifier_seq(ps, &cv_flags);
    fn->cv_flags = cv_flags;

    eat_ref_qualifier(ps);
    eat_exception_specification(ps);
    eat_attribute_specifier_seq(ps);
    return true;
}
bool eat_noptr_abstract_declarator2(parse_state& ps, declarator* d) {
    if (eat_parameters_and_qualifiers(ps, d)) {
        return true;
    }
    token tok;
    if (eat_token(ps, "[", &tok)) {
        // TODO: constant-expression
        eat_balanced_token_seq(ps);
        if (!eat_token(ps, "]", &tok)) {
            throw parse_exception("Expected a ']'", tok);
        }
        eat_attribute_specifier_seq(ps);
        return true;
    }
    return false;
}
bool eat_ptr_abstract_declarator(parse_state& ps, declarator* d);
bool eat_noptr_abstract_declarator(parse_state& ps, declarator* d) {
    if(accept(ps, "(")) {
        if (!eat_ptr_abstract_declarator(ps, d)) {
            throw parse_exception("Expected a declarator", ps.next_token());
        }
        expect(ps, ")");
    }

    int elements_eaten = 0;
    while (true) {
        if (eat_array_braces(ps)) {
            eat_attribute_specifier_seq(ps);
            if (!d->decl_type_.empty()) {
                if (d->decl_type_.get_last()->cast_to<type_id_part_function>()) {
                    throw parse_exception("Function returning an array is not allowed", ps.next_token());
                }
            }
            auto arr = d->decl_type_.push_back<type_id_part_array>();
            ++elements_eaten;
            continue;
        }
        if (eat_parameters_and_qualifiers(ps, d)) {
            ++elements_eaten;
            continue;
        }

        if (elements_eaten == 0) {
            return false;
        } else {
            return true;
        }
    }
    return true;
}
bool eat_ptr_abstract_declarator(parse_state& ps, declarator* d) {
    if (eat_noptr_abstract_declarator(ps, d)) {
        return true;
    }
    ptr_decl_part ptr_part;
    if (eat_ptr_operator(ps, &ptr_part)) {
        eat_ptr_abstract_declarator(ps, d);

        if (ptr_part.type == PTR_DECL_PTR) {
            auto part = d->decl_type_.push_back<type_id_part_ptr>();
            part->cv_flags = ptr_part.cv_flags;
        } else if(ptr_part.type == PTR_DECL_LVALUE_REF) {
            if (!d->decl_type_.empty()) {
                if (d->decl_type_.get_last()->cast_to<type_id_part_ptr>()) {
                    throw parse_exception("Pointer to reference is not allowed", ps.next_token());
                } else if (d->decl_type_.get_last()->cast_to<type_id_part_ref>()) {
                    throw parse_exception("Reference to reference is not allowed", ps.next_token());
                }
            }
            auto part = d->decl_type_.push_back<type_id_part_ref>();
            part->cv_flags = ptr_part.cv_flags;
        } else if(ptr_part.type == PTR_DECL_RVALUE_REF) {
            if (!d->decl_type_.empty()) {
                if (d->decl_type_.get_last()->cast_to<type_id_part_ptr>()) {
                    throw parse_exception("Pointer to reference is not allowed", ps.next_token());
                } else if (d->decl_type_.get_last()->cast_to<type_id_part_ref>()) {
                    throw parse_exception("Reference to reference is not allowed", ps.next_token());
                }
            }
            auto part = d->decl_type_.push_back<type_id_part_ref>();
            part->cv_flags = ptr_part.cv_flags;
        }
        return true;
    }
    return false;
}
bool eat_noptr_abstract_pack_declarator(parse_state& ps, declarator* d) {
    token tok;
    if (eat_noptr_abstract_declarator2(ps, d)) {
        // ==    
    } else if (eat_token(ps, "...", &tok)) {
        // ==
    } else {
        return false;
    }

    while (eat_noptr_abstract_declarator2(ps, d)) {

    }
    return true;
}
bool eat_abstract_pack_declarator(parse_state& ps, declarator* d) {
    if (eat_noptr_abstract_pack_declarator(ps, d)) {
        return true;
    }
    ptr_decl_part ptr_part_;
    if (eat_ptr_operator(ps, &ptr_part_)) {
        if (!eat_abstract_pack_declarator(ps, d)) {
            throw parse_exception("Expected an abstract_pack_declarator", ps.next_token());
        }
        return true;
    }
    return false;
}
bool eat_trailing_return_type(parse_state& ps, decl_spec& dspec, decl_type& dtype, declarator* d);
bool eat_abstract_declarator(parse_state& ps, decl_spec& dspec, decl_type& dtype, declarator* d) {
    if (eat_noptr_abstract_declarator(ps, d)) {
        if (d->decl_type_.get_last() && d->decl_type_.get_last()->cast_to<type_id_part_function>()) {
            decl_spec dspec2;
            decl_type dtype2;
            declarator d2;
            if (eat_trailing_return_type(ps, dspec2, dtype2, &d2)) {
                if ((dtype.type_flags & fl_auto) != fl_auto || (dspec.decl_flags & (DECL_CV_CONST | DECL_CV_VOLATILE)) != 0) {
                    throw parse_exception("Trailing return type requires 'auto' by itself before the function name", ps.next_token());
                }
                if (dtype2.type_flags != fl_auto) {
                    auto tid_part = d2.decl_type_.push_back<type_id_part_object>();
                    tid_part->object_type_ = std::move(dtype2);
                    tid_part->decl_specifiers = std::move(dspec2);
                }
                d->decl_type_.move_merge_back(d2.decl_type_);
            }
        }
        return true;
    }
    if (eat_ptr_abstract_declarator(ps, d)) {
        return true;
    }
    if (eat_abstract_pack_declarator(ps, d)) {
        return true;
    }
    return false;
}
bool eat_trailing_return_type(parse_state& ps, decl_spec& dspec, decl_type& dtype, declarator* d) {
    token tok;
    if (!eat_token(ps, "->", &tok)) {
        return false;
    }
    if (!eat_trailing_type_specifier_seq(ps, dspec, dtype)) {
        throw parse_exception("Expected a trailing_return_type_seq", ps.next_token());
    }
    // TODO:
    eat_declarator(ps, dspec, dtype, d, 0, true);
    return true;
}

bool eat_id_expression(parse_state& ps, declarator* d) {
    /* TODO:
    id-expression:
        unqualified-id
        qualified-id
    unqualified-id:
        identifier
        operator-function-id
        conversion-function-id
        literal-operator-id
        ~ class-name
        ~ decltype-specifier
        template-id*/
    token tok;
    if (eat_token(ps, tt_identifier, &tok)) {
        d->name = tok.str;
        d->file = tok.file;
        return true;
    }
    return false;
}
bool eat_declarator_id(parse_state& ps, declarator* d) {
    ps.push_rewind_point();
    token tok;
    eat_token(ps, "...", &tok);
    if (!eat_id_expression(ps, d)) {
        ps.rewind_();
        return false;
    }
    ps.pop_rewind_point();
    return true;
}

bool eat_ptr_declarator(parse_state& ps, decl_spec& dspec, decl_type& dtype, declarator* d, bool is_abstract);
bool eat_noptr_declarator(parse_state& ps, decl_spec& dspec, decl_type& dtype, declarator* d, bool is_abstract) {
    token tok;
    if (eat_declarator_id(ps, d)) {
        eat_attribute_specifier_seq(ps);
    } else if(eat_token(ps, "(", &tok)) {
        if (!eat_ptr_declarator(ps, dspec, dtype, d, is_abstract)) {
            throw parse_exception("Expected a declarator", ps.next_token());
        }

        if (!eat_token(ps, ")", &tok)) {
            throw parse_exception("Expected a ')'", tok);
        }
    } else if(!is_abstract) {
        return false;
    }

    while (true) {
        if (eat_array_braces(ps)) {
            eat_attribute_specifier_seq(ps);
            if (!d->decl_type_.empty()) {
                if (d->decl_type_.get_last()->cast_to<type_id_part_function>()) {
                    throw parse_exception("Function returning an array is not allowed", ps.next_token());
                }
            }
            auto arr = d->decl_type_.push_back<type_id_part_array>();
            continue;
        }
        if (eat_parameters_and_qualifiers(ps, d)) {
            continue;
        }
        return true;
    }
    return true;
}
bool eat_ptr_declarator(parse_state& ps, decl_spec& dspec, decl_type& dtype, declarator* d, bool is_abstract) {
    ptr_decl_part ptr_part;
    if (eat_ptr_operator(ps, &ptr_part)) {
        if (!eat_ptr_declarator(ps, dspec, dtype, d, is_abstract)) {
            if (!is_abstract) {
                throw parse_exception("Expected ptr_declarator", ps.next_token());
            }
        }
        if (ptr_part.type == PTR_DECL_PTR) {
            auto part = d->decl_type_.push_back<type_id_part_ptr>();
            part->cv_flags = ptr_part.cv_flags;
        } else if(ptr_part.type == PTR_DECL_LVALUE_REF) {
            if (!d->decl_type_.empty()) {
                if (d->decl_type_.get_last()->cast_to<type_id_part_ptr>()) {
                    throw parse_exception("Pointer to reference is not allowed", ps.next_token());
                } else if (d->decl_type_.get_last()->cast_to<type_id_part_ref>()) {
                    throw parse_exception("Reference to reference is not allowed", ps.next_token());
                }
            }
            auto part = d->decl_type_.push_back<type_id_part_ref>();
            part->cv_flags = ptr_part.cv_flags;
        } else if(ptr_part.type == PTR_DECL_RVALUE_REF) {
            if (!d->decl_type_.empty()) {
                if (d->decl_type_.get_last()->cast_to<type_id_part_ptr>()) {
                    throw parse_exception("Pointer to reference is not allowed", ps.next_token());
                } else if (d->decl_type_.get_last()->cast_to<type_id_part_ref>()) {
                    throw parse_exception("Reference to reference is not allowed", ps.next_token());
                }
            }
            auto part = d->decl_type_.push_back<type_id_part_ref>();
            part->cv_flags = ptr_part.cv_flags;
        }
        return true;
    } else if (eat_noptr_declarator(ps, dspec, dtype, d, is_abstract)) {
        if (d->decl_type_.get_last() && d->decl_type_.get_last()->cast_to<type_id_part_function>()) {
            decl_spec dspec2;
            decl_type dtype2;
            declarator d2;
            if (eat_trailing_return_type(ps, dspec2, dtype2, &d2)) {
                if ((dtype.type_flags & fl_auto) != fl_auto || (dspec.decl_flags & (DECL_CV_CONST | DECL_CV_VOLATILE)) != 0) {
                    throw parse_exception("Trailing return type requires 'auto' by itself before the function name", ps.next_token());
                }/*
                if (dtype2.type_flags != fl_auto) {
                    d2.decl_type_.push_back<type_id_part_object>()->object_type_ = std::move(dtype2);
                }*/
                d->decl_type_.move_merge_back(d2.decl_type_);
            }
        }
        return true;
    }

    return false;
}

bool eat_declarator(parse_state& ps, decl_spec& dspec, decl_type& dtype, declarator* d, std::shared_ptr<symbol>* out_sym, bool is_abstract) {
    bool success = false;
    if (eat_ptr_declarator(ps, dspec, dtype, d, is_abstract)) {
        success = true;
    } else if (eat_noptr_declarator(ps, dspec, dtype, d, is_abstract)) {
        if (d->decl_type_.get_last() && d->decl_type_.get_last()->cast_to<type_id_part_function>()) {
            decl_spec dspec2;
            decl_type dtype2;
            declarator d2;
            if (eat_trailing_return_type(ps, dspec2, dtype2, &d2)) {
                if ((dtype.type_flags & fl_auto) != fl_auto || (dspec.decl_flags & (DECL_CV_CONST | DECL_CV_VOLATILE)) != 0) {
                    throw parse_exception("Trailing return type requires 'auto' by itself before the function name", ps.next_token());
                }/*
                if (dtype2.type_flags != fl_auto) {
                    d2.decl_type_.push_back<type_id_part_object>()->object_type_ = std::move(dtype2);
                }*/
                d->decl_type_.move_merge_back(d2.decl_type_);
            }
        }
        success = true;
    } else if(is_abstract) {
        // TODO: abstract-pack-declarator
    }

    if (success) {
        if ((dtype.type_flags & fl_user_defined) && dtype.user_defined_type->is<symbol_typedef>()) {
            symbol_typedef* sym_typedef = (symbol_typedef*)dtype.user_defined_type.get();
            d->decl_type_.push_back(sym_typedef->type_id_);
        } else if (d->decl_type_.get_last() == 0 || d->decl_type_.get_last()->cast_to<type_id_part_object>() == 0) {
            auto t = d->decl_type_.push_back<type_id_part_object>();
            t->object_type_ = dtype;
            t->decl_specifiers = dspec;
        }

        if (is_abstract) {
            // Don't put abstract declarators into the symbol table
            return true;
        }

        if (dspec.decl_flags & DECL_TYPEDEF) {
            auto existing = ps.get_current_scope()->get_symbol(d->name, LOOKUP_FLAG_TYPEDEF);
            auto existing_typedef = (symbol_typedef*)existing.get();
            if (existing_typedef && existing_typedef->type_id_ != d->decl_type_) {
                throw parse_exception("Typedef redeclaration with different types", ps.next_token());
            }
            std::shared_ptr<symbol_typedef> sptr(new symbol_typedef);
            sptr->name = d->name;
            sptr->type_id_ = d->decl_type_;
            sptr->file = d->file;
            d->sym = sptr;
            ps.add_symbol(sptr);
            if (out_sym) {
                *out_sym = sptr;
            }
        } else if (d->is_func()) {
            auto sym_fn = ps.get_current_scope()->get_symbol(d->name, LOOKUP_FLAG_OBJECT | LOOKUP_FLAG_FUNCTION);
            if (sym_fn && sym_fn->is<symbol_object>()) {
                throw parse_exception("Name already declared as an object", ps.next_token());
            }
            if (!sym_fn) {
                sym_fn = std::shared_ptr<symbol_function>(new symbol_function);
                sym_fn->name = d->name;
                sym_fn->file = d->file;
                ps.add_symbol(sym_fn);
            }
            symbol_function* sym_fn_ = ((symbol_function*)sym_fn.get());
            std::shared_ptr<symbol_func_overload> overload = sym_fn_->get_or_create_overload(d->decl_type_);
            if (!overload) {
                assert(false);
            }
            d->sym = overload;
            if (out_sym) {
                *out_sym = overload;
            }
        } else { // object
            auto existing = ps.get_current_scope()->get_symbol(d->name, LOOKUP_FLAG_OBJECT | LOOKUP_FLAG_FUNCTION);
            if (existing && existing->is<symbol_object>()) {
                throw parse_exception("Redefinition of an object", ps.next_token());
            } else if(existing && existing->is<symbol_function>()) {
                throw parse_exception("Name already declared as a function", ps.next_token());
            }
            std::shared_ptr<symbol_object> sptr(new symbol_object);
            sptr->name = d->name;
            sptr->set_defined(true);
            sptr->type_id_ = d->decl_type_;
            sptr->file = d->file;
            d->sym = sptr;
            ps.add_symbol(sptr);
            if (out_sym) {
                *out_sym = sptr;
            }
        }
        return true;
    }

    return false;
}
bool eat_initializer(parse_state& ps) {
    if (eat_brace_or_equal_initializer(ps)) {
        return true;
    }
    token tok;
    if (eat_token(ps, "(", &tok)) {
        // TODO: expression-list
        eat_balanced_token_seq(ps);

        if (!eat_token(ps, ")", &tok)) {
            throw parse_exception("Expected a ')'", tok);
        }
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
    if (eat_token(ps, "0", &tok)) {
        ps.pop_rewind_point();
        return true;
    }
    ps.rewind_();
    return false;
}
bool eat_init_declarator(parse_state& ps, decl_spec& dspec, decl_type& dtype, declarator* d) {
    if (!eat_declarator(ps, dspec, dtype, d, 0)) {
        return false;
    }

    if (d->is_func() && (dspec.decl_flags & DECL_FN_VIRTUAL)) {
        // TODO: Only for member functions
        eat_pure_specifier(ps);
    } else if(!d->is_func()) {
        eat_initializer(ps);
    }
    return true;
}
bool eat_init_declarator_list(parse_state& ps, decl_spec& dspec, decl_type& dtype, std::vector<declarator>* list) {
    declarator d;
    if (!eat_init_declarator(ps, dspec, dtype, &d)) {
        return false;
    }
    list->push_back(std::move(d));
    while (true) {
        token tok;
        if (!eat_token(ps, ",", &tok)) {
            return true;
        }
        declarator d;
        if (!eat_init_declarator(ps, dspec, dtype, &d)) {
            throw parse_exception("Expected an init-declarator", ps.next_token());
        }
        list->push_back(std::move(d));
    }
}
bool eat_declarator_list(parse_state& ps, decl_spec& dspec, decl_type& dtype, std::vector<declarator>* list) {
    declarator d;
    if (!eat_declarator(ps, dspec, dtype, &d, 0)) {
        return false;
    }
    list->push_back(std::move(d));
    while (true) {
        token tok;
        if (!eat_token(ps, ",", &tok)) {
            return true;
        }
        declarator d;
        if (!eat_declarator(ps, dspec, dtype, &d, 0)) {
            throw parse_exception("Expected a declarator", ps.next_token());
        }
        list->push_back(std::move(d));
    }
}
bool eat_virt_specifier(parse_state& ps) {
    token tok;
    if (eat_token(ps, "override", &tok)) {
        return true;
    }
    if (eat_token(ps, "final", &tok)) {
        return true;
    }
    return false;
}
bool eat_virt_specifier_seq(parse_state& ps) {
    if (!eat_virt_specifier(ps)) {
        return false;
    }
    while (eat_virt_specifier(ps)) {

    }
    return true;
}

bool eat_compound_statement(parse_state& ps, declarator* d) {
    token tok;
    if (!eat_token(ps, "{", &tok)) {
        return false;
    }
    if (d && d->sym->is_defined()) {
        throw parse_exception("Function already defined", tok);
    }

    // TODO:
    eat_balanced_token_seq(ps);

    if (!eat_token(ps, "}", &tok)) {
        throw parse_exception("Expected a '}'", tok);
    }
    return true;
}
bool eat_handler(parse_state& ps, declarator* d) {
    if (!eat_keyword(ps, kw_catch)) {
        return false;
    }
    token tok;
    if (!eat_token(ps, "(", &tok)) {
        throw parse_exception("Expected an '('", tok);
    }
    // TODO:
    eat_balanced_token_seq(ps);

    if (!eat_token(ps, ")", &tok)) {
        throw parse_exception("Expected a ')'", tok);
    }
    if (!eat_compound_statement(ps, d)) {
        throw parse_exception("Expected a compound_statement", ps.next_token());
    }
    return true;
}
bool eat_handler_seq(parse_state& ps, declarator* d) {
    if (!eat_handler(ps, d)) {
        return false;
    }
    while (eat_handler(ps, d)) {

    }
    return true;
}
bool eat_braced_init_list(parse_state& ps) {
    token tok;
    if (!eat_token(ps, "{", &tok)) {
        return false;
    }

    // TODO:
    eat_balanced_token_seq(ps);

    if (!eat_token(ps, "}", &tok)) {
        throw parse_exception("Expected a '}'", tok);
    }
    return true;
}
bool eat_mem_initializer_id(parse_state& ps) {
    if (eat_class_or_decltype(ps)) {
        return true;
    }
    if (eat_identifier(ps)) {
        return true;
    }
    return false;
}
bool eat_mem_initializer(parse_state& ps) {
    if (!eat_mem_initializer_id(ps)) {
        return false;
    }
    token tok;
    if (eat_token(ps, "(", &tok)) {
        // TODO:
        eat_balanced_token_seq(ps);
        if (!eat_token(ps, ")", &tok)) {
            throw parse_exception("Expected a ')'", tok);
        }
        return true;
    } else if(eat_braced_init_list(ps)) {
        return true;
    } else {
        throw parse_exception("Expected an initialization", ps.next_token());
    }
}
bool eat_mem_initializer_list(parse_state& ps) {
    if (!eat_mem_initializer(ps)) {
        return false;
    }
    token tok;
    eat_token(ps, "...", &tok);
    while (true) {
        if (!eat_token(ps, ",", &tok)) {
            return true;
        }
        if (!eat_mem_initializer_list(ps)) {
            throw parse_exception("Expected a mem_initializer_list", ps.next_token());
        }
    }
}
bool eat_ctor_initializer(parse_state& ps, declarator* d) {
    token tok;
    if (!eat_token(ps, ":", &tok)) {
        return false;
    }
    if (d->sym->is_defined()) {
        throw parse_exception("Function already defined", ps.next_token());
    }

    if (!eat_mem_initializer_list(ps)) {
        throw parse_exception("Expected a mem_initializer_list", ps.next_token());
    }
    return true;
}
bool eat_function_try_block(parse_state& ps, declarator* d) {
    if (!eat_keyword(ps, kw_try)) {
        return false;
    }
    if (d->sym->is_defined()) {
        throw parse_exception("Function already defined", ps.next_token());
    }

    eat_ctor_initializer(ps, d);
    if (!eat_compound_statement(ps, d)) {
        throw parse_exception("Expected a compound_statement", ps.next_token());
    }
    if (!eat_handler_seq(ps, d)) {
        throw parse_exception("Expected a handler_seq", ps.next_token());
    }
    return true;
}
bool eat_function_body(parse_state& ps, declarator* d) {
    if (eat_ctor_initializer(ps, d)) {
        if (!eat_compound_statement(ps, d)) {
            throw parse_exception("Expected a compound_statement", ps.next_token());
        }
        d->sym->set_defined(true);
        return true;
    }
    if (eat_compound_statement(ps, d)) {
        d->sym->set_defined(true);
        return true;
    }
    if (eat_function_try_block(ps, d)) {
        d->sym->set_defined(true);
        return true;
    }
    token tok;
    if (eat_token(ps, "=", &tok)) {
        if (d->sym->is_defined()) {
            throw parse_exception("Function already defined", tok);
        }
        if (eat_token(ps, "default", &tok)) {

        } else if(eat_token(ps, "delete", &tok)) {
            
        } else {
            throw parse_exception("Expected a default or delete", tok);
        }
        if (!eat_token(ps, ";", &tok)) {
            throw parse_exception("Expected a ';'", tok);
        }
        d->sym->set_defined(true);
        return true;
    }
    return false;
}
bool eat_function_definition(parse_state& ps) {
    ps.push_rewind_point();

    eat_attribute_specifier_seq(ps);
    decl_spec dspec;
    decl_type dtype;
    eat_decl_specifier_seq(ps, dspec, dtype);
    declarator d;
    if (!eat_declarator(ps, dspec, dtype, &d, 0)) {
        ps.rewind_();
        return false;
    }
    eat_virt_specifier_seq(ps);
    if (!eat_function_body(ps, &d)) {
        ps.rewind_();
        return false;
    }
    ps.pop_rewind_point();
    return true;
}