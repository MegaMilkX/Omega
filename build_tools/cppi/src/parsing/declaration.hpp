#pragma once

#include "parse_state.hpp"


// ignore_free_standing_attribs is necessary because when this function is
// used in eat_cppi_block(), it needs to be able to see attribute specifiers
// to catch [[cppi_end]]
// without the flag eat_declaration() will consume them,
// and eat_cppi_block() will not stop until the end of file
bool eat_declaration(parse_state& ps, bool ignore_free_standing_attribs = false);
bool eat_type_id(parse_state& ps, type_id& tid);