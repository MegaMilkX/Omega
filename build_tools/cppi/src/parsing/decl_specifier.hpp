#pragma once

#include "parse_state.hpp"
#include "class_specifier.hpp"
#include "parse_exception.hpp"

#include "type_id.hpp"

struct decl_or_type_specifier_seq {
    decl_spec decl_spec_;
    decl_type type_;

    void dbg_print_unqualified_type() const {
        type_.dbg_print();
    }
};

bool eat_decl_specifier(parse_state& ps, decl_spec& decl_spec_, decl_type& object_type_);
bool eat_decl_specifier_seq(parse_state& ps, decl_spec& ds, decl_type& dt);
