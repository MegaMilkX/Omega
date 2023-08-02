#pragma once

#include <vector>
#include "pp_types.hpp"
#include "char_provider.hpp"


int pp_parse_token(char_provider& rs, PP_TOKEN* out, bool include_header_name_token = false);
int parse_into_pp_tokens(const char* text, int text_len, std::vector<PP_TOKEN>& out, const PP_FILE* file);
int parse_into_pp_tokens_and_header_name_skip_first_whitespace(const char* text, int text_len, std::vector<PP_TOKEN>& out, const PP_FILE* file);
