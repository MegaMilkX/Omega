#pragma once

#include "pp_state.hpp"


int try_macro_expansion(pp_state& pps, const PP_TOKEN& expanded_tok, std::vector<PP_TOKEN>& list, bool keep_defined_op_param = false);
