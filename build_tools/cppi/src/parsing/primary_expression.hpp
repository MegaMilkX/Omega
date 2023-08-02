#pragma once

#include "parse_state.hpp"

bool eat_nested_name_specifier(
    parse_state& ps, 
    std::shared_ptr<symbol_table>& scope, 
    bool as_placeholder = false
);
bool eat_qualified_id(parse_state& ps);
bool eat_unqualified_id(parse_state& ps);
bool eat_id_expression(parse_state& ps);
bool eat_primary_expression(parse_state& ps);
