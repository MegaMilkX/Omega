#pragma once

#include "parse_state.hpp"


bool eat_declaration(parse_state& ps);
bool eat_type_id(parse_state& ps, type_id& tid);