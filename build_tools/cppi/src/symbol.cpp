#include "symbol.hpp"

#include "parse_state.hpp"


void symbol::copy_to(symbol* dest) const {
    dest->file = file;
    dest->name = name;
    dest->global_qualified_name = global_qualified_name;
    if(nested_symbol_table) {
        dest->nested_symbol_table = nested_symbol_table->clone();
    }
    dest->attrib_spec.merge(attrib_spec);
    dest->defined = defined;
    dest->no_reflect = no_reflect;
}


void symbol_namespace::dbg_print() const {
    dbg_printf_color("namespace %s\n", DBG_RED | DBG_GREEN | DBG_BLUE, global_qualified_name.c_str());
}