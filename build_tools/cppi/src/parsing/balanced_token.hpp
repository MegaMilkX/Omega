#pragma once

#include "parse_state.hpp"


bool eat_balanced_token(parse_state& ps);
bool eat_balanced_token_seq(parse_state& ps);
bool eat_balanced_token_except(parse_state& ps, const std::initializer_list<const char*>& exceptions);
bool eat_balanced_token_seq_except(parse_state& ps, const std::initializer_list<const char*>& exceptions);