#include "pp_state.hpp"


bool pp_eat_until_newiline(pp_state& pps);
bool pp_include_file(pp_state& pps, const char* path);
bool pp_eat_directive(pp_state& pps);
