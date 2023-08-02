#pragma once

#include "parse_state.hpp"



struct enum_specifier_node;
std::unique_ptr<enum_specifier_node> eat_enum_specifier2(parse_state& ps);

bool eat_enum_key(parse_state& ps, e_enum_key& key);
bool eat_enum_base(parse_state& ps);

std::shared_ptr<symbol> eat_enum_specifier(parse_state& ps);
std::shared_ptr<symbol> eat_enum_specifier_limited(parse_state& ps);
