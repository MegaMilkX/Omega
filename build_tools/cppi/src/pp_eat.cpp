#include "pp_constexpr.hpp"
#include "pp_macro_expansion.hpp"
#include "pp_exception.hpp"

bool pp_eat_until_newiline(pp_state& pps) {
    PP_TOKEN tok = pps.next_pp_token(true);
    while (tok.type != NEW_LINE) {
        tok = pps.next_pp_token(true);
    }
    return true;
}/*
bool pp_include_file(pp_state& pps, const char* path) {
    std::string fname = path;
    FILE* f = fopen(fname.c_str(), "rb");
    if (!f) {
        return false;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize <= 0) {
        return false;
    }

    std::vector<char> buf;
    buf.resize(fsize);
    fread(buf.data(), fsize, 1, f);

    pps.include_file(buf.data(), buf.size(), path);
    return true;
}*/
bool pp_eat_directive(pp_state& pps) {
    pps.set_pp_rewind_point();
    PP_TOKEN tok = pps.next_pp_token(true);
    if (tok.type != PP_OP_OR_PUNC || tok.str != "#") {
        pps.pp_rewind();
        return false;
    }

    tok = pps.next_pp_token(true);
    if (tok.type != IDENTIFIER) {
        while (tok.type != NEW_LINE) {
            tok = pps.next_pp_token(true);
        }
        return true;
    }

    auto current_group = pps.get_current_group();

    if (tok.str == "if") {
        intmax_t result = 0;
        if (!pp_eat_constant_expression(pps, &result)) {
            throw pp_exception("Expected a constant-expression", pps);
        }
        bool group_enabled = result != 0;
        pps.push_group(PP_GROUP{ PP_GROUP_IF, group_enabled && current_group.is_enabled, !group_enabled && current_group.is_enabled });

        tok = pps.next_pp_token(true);
        while (tok.type != NEW_LINE) {
            tok = pps.next_pp_token(true);
        }
        return true;
    } else if(tok.str == "ifdef") {
        tok = pps.next_pp_token(true);
        if (tok.type != IDENTIFIER) {
            throw pp_exception("Expected an identifier", tok);
        }
        bool group_enabled = false;
        auto it = pps.macros.find(tok.str);
        if (it != pps.macros.end()) {
            group_enabled = true;
        }
        pps.push_group(PP_GROUP{ PP_GROUP_IF, group_enabled && current_group.is_enabled, !group_enabled && current_group.is_enabled });

        tok = pps.next_pp_token(true);
        while (tok.type != NEW_LINE) {
            tok = pps.next_pp_token(true);
        }
        return true;
    } else if(tok.str == "ifndef") {
        tok = pps.next_pp_token(true);
        if (tok.type != IDENTIFIER) {
            throw pp_exception("Expected an identifier", tok);
        }
        bool group_enabled = true;
        auto it = pps.macros.find(tok.str);
        if (it != pps.macros.end()) {
            group_enabled = false;
        }
        pps.push_group(PP_GROUP{ PP_GROUP_IF, group_enabled && current_group.is_enabled, !group_enabled && current_group.is_enabled });

        tok = pps.next_pp_token(true);
        while (tok.type != NEW_LINE) {
            tok = pps.next_pp_token(true);
        }
        return true;
    } else if(tok.str == "elif") {
        intmax_t result = 0;
        if (!pp_eat_constant_expression(pps, &result)) {
            throw pp_exception("Expected a constant expression", pps);
        }
        if (current_group.type != PP_GROUP_IF && current_group.type != PP_GROUP_ELIF) {
            throw pp_exception("#elif only allowed after #if, #ifdef, #ifndef, #elif directives", tok);
        }
        bool group_enabled = result != 0 && current_group.can_enable_else;
        pps.push_group(PP_GROUP{ PP_GROUP_ELIF, group_enabled, !(result != 0) && current_group.can_enable_else });

        tok = pps.next_pp_token(true);
        while (tok.type != NEW_LINE) {
            tok = pps.next_pp_token(true);
        }
        return true;
    } else if(tok.str == "else") {
        if (current_group.type != PP_GROUP_IF && current_group.type != PP_GROUP_ELIF) {
            throw pp_exception("#else only allowed after #if, #ifdef, #ifndef, #elif directives", pps);
        }

        pps.push_group(PP_GROUP{ PP_GROUP_ELSE, current_group.can_enable_else, false });

        tok = pps.next_pp_token(true);
        while (tok.type != NEW_LINE) {
            tok = pps.next_pp_token(true);
        }
        return true;
    } else if(tok.str == "endif") {
        while (true) {
            current_group = pps.get_current_group();
            if (current_group.type == PP_GROUP_ELSE
                || current_group.type == PP_GROUP_ELIF ) {
                pps.pop_group();
                continue;
            } else if(current_group.type == PP_GROUP_IF) {
                pps.pop_group();
                break;
            } else {
                throw pp_exception("#endif directive not allowed outside of an #if section", pps);
            }
        }

        tok = pps.next_pp_token(true);
        while (tok.type != NEW_LINE) {
            tok = pps.next_pp_token(true);
        }
        return true;
    } else if(tok.str == "include") {
        if (!current_group.is_enabled) {
            pp_eat_until_newiline(pps);
            return true;
        }

        std::string str = pps.next_characters_until_new_line();
        std::vector<PP_TOKEN> tokens;
        int tok_count = parse_into_pp_tokens_and_header_name_skip_first_whitespace(str.c_str(), str.size(), tokens, pps.get_current_file());
        if (tok_count == 0) {
            throw pp_exception("Expected a file name", tok);
        }
        std::string tokstr;
        if (tokens[0].type != HEADER_NAME) {
            pps.insert_pp_tokens(tokens, false);
            tok = pps.next_pp_token(true);
            while (tok.type != NEW_LINE) {
                std::vector<PP_TOKEN> list;
                if (try_macro_expansion(pps, tok, list)) {
                    for (auto& t : list) {
                        if (t.type == WHITESPACE) {
                            t.str = " ";
                        }
                        tokstr.insert(tokstr.end(), t.str.begin(), t.str.end());
                    }
                } else {
                    if (tok.type == WHITESPACE) {
                        tok.str = " ";
                    }
                    tokstr.insert(tokstr.end(), tok.str.begin(), tok.str.end());
                }
                tok = pps.next_pp_token(false);
            }

            tokens.clear();
            int tok_count = parse_into_pp_tokens_and_header_name_skip_first_whitespace(tokstr.c_str(), tokstr.size(), tokens, pps.get_current_file());
            if (tok_count == 0) {
                throw pp_exception("Macro expanded include directive line produced no tokens", tok);
            }
        }
        if (tokens.empty()) {
            throw pp_exception("include directive file name is empty", tok);
        }
        if (tokens[0].type != HEADER_NAME) {
            if (pps.ignore_missing_includes) {
                emit_warning("include directive is not followed by a header_name, ignoring", &pps);
                return true;
            } else {
                throw pp_exception("include directive is not followed by a header_name", tok);
            }
        }
        
        std::string header_name = tokens[0].str;
        bool angled_brackets = header_name[0] == '<';

        // Removing the " or < characters
        header_name.pop_back();
        header_name.erase(header_name.begin());

        if (!pps.include_file(header_name, angled_brackets)) {
            throw pp_exception("Can't open include file '%s'", tok, header_name.c_str());
        }

        return true;
    } else if(tok.str == "define") {
        if (!current_group.is_enabled) {
            pp_eat_until_newiline(pps);
            return true;
        }

        tok = pps.next_pp_token(true);
        if (tok.type != IDENTIFIER) {
            throw pp_exception("Expected an identifier", tok);
        }
        PP_MACRO m;
        m.has_va_args = false;
        std::string macro_name = tok.str;
        std::vector<PP_TOKEN> replacement_list;
        tok = pps.next_pp_token(false);
        if (tok.type == PP_OP_OR_PUNC && tok.str == "(") {
            // Function like macro
            tok = pps.next_pp_token(true);
            if (tok.type == IDENTIFIER) {
                if (tok.str == "__VA_ARGS__") {
                    throw pp_exception("__VA_ARGS__ can't be used as a macro parameter", tok);
                }
                std::vector<PP_TOKEN> identifier_list;
                identifier_list.push_back(tok);
                tok = pps.next_pp_token(true);
                bool trailing_comma = false;
                while (tok.type == PP_OP_OR_PUNC && tok.str == ",") {
                    tok = pps.next_pp_token(true);
                    if (tok.type != IDENTIFIER) {
                        trailing_comma = true;
                        break;
                    }
                    if (tok.str == "__VA_ARGS__") {
                        throw pp_exception("__VA_ARGS__ can't be used as a macro parameter", tok);
                    }
                    identifier_list.push_back(tok);
                    tok = pps.next_pp_token(true);
                }

                m.params_ = identifier_list;

                if (trailing_comma) {
                    if (tok.type == PP_OP_OR_PUNC && tok.str == "...") {
                        m.has_va_args = true;
                        PP_TOKEN va_args_param;
                        va_args_param.type = IDENTIFIER;
                        va_args_param.str = "__VA_ARGS__";
                        va_args_param.is_macro_expandable = true;
                        m.params_.push_back(va_args_param);
                        tok = pps.next_pp_token(true);
                    } else {
                        throw pp_exception("Expected an identifier or elipsis", tok);
                    }
                }
            } else if(tok.type == PP_OP_OR_PUNC && tok.str == "...") {
                m.has_va_args = true;
                PP_TOKEN va_args_param;
                va_args_param.type = IDENTIFIER;
                va_args_param.str = "__VA_ARGS__";
                va_args_param.is_macro_expandable = true;
                m.params_.push_back(va_args_param);
                tok = pps.next_pp_token(true);
            }

            if (tok.type != PP_OP_OR_PUNC || tok.str != ")") {
                throw pp_exception("Expected a closing parenthesis", tok);
            }
            tok = pps.next_pp_token(true);
            m.is_function_like = true;
        } else {
            // Object macro
            m.is_function_like = false;
        }

        if (tok.type == WHITESPACE) {
            tok = pps.next_pp_token(false);
        }
        if (tok.type == PP_OP_OR_PUNC && tok.str == "##") {
            throw pp_exception("## may not appear first in the replacement list", tok);
        }
        while (tok.type != NEW_LINE) {
            if (tok.type == PP_OP_OR_PUNC) {
                if (tok.str == "##") {
                    if (!replacement_list.empty() && replacement_list.back().type == WHITESPACE) {
                        replacement_list.pop_back();
                    }
                    if (!replacement_list.empty() && replacement_list.back().type == MACRO_PARAM) {
                        for (auto& p : m.params_) {
                            if (p.str == replacement_list.back().str) {
                                p.is_macro_expandable = false;
                                break;
                            }
                        }
                    }
                    replacement_list.push_back(tok);
                    tok = pps.next_pp_token(true);
                    if (tok.type == NEW_LINE) {
                        throw pp_exception("## may not appear last in the replacement list", tok);
                    }
                    if (tok.type == IDENTIFIER) {
                        for (int i = 0; i < m.params_.size(); ++i) {
                            auto& p = m.params_[i];
                            if (p.str == tok.str) {
                                tok.type = MACRO_PARAM;
                                tok.param_idx = i;
                                p.is_macro_expandable = false;
                                break;
                            }
                        }
                    }
                    replacement_list.push_back(tok);
                } else if (m.is_function_like && tok.str == "#") {
                    replacement_list.push_back(tok);
                    tok = pps.next_pp_token(true);
                    if (tok.type != IDENTIFIER) {
                        throw pp_exception("Expected an identifier token", tok);
                    }
                    bool is_param_name = false;
                    for (int i = 0; i < m.params_.size(); ++i) {
                        auto& p = m.params_[i];
                        if (p.str == tok.str) {
                            tok.type = MACRO_PARAM;
                            tok.param_idx = i;
                            p.is_macro_expandable = false;
                            is_param_name = true;
                            break;
                        }
                    }
                    if (!is_param_name) {
                        throw pp_exception("Expected a macro parameter name", tok);
                    }

                    replacement_list.push_back(tok);
                } else {
                    replacement_list.push_back(tok);
                }
            } else if(tok.type == IDENTIFIER) {
                for (int i = 0; i < m.params_.size(); ++i) {
                    auto& p = m.params_[i];
                    if (p.str == tok.str) {
                        tok.type = MACRO_PARAM;
                        tok.param_idx = i;
                        break;
                    }
                }
                replacement_list.push_back(tok);
            } else {
                replacement_list.push_back(tok);
            }
            tok = pps.next_pp_token();
        }
        if (!replacement_list.empty() && replacement_list.back().type == WHITESPACE) {
            replacement_list.pop_back();
        }
        m.replacement_list_ = replacement_list;
        pps.macros[macro_name] = m;
        return true;
    } else if(tok.str == "undef") {
        if (!current_group.is_enabled) {
            pp_eat_until_newiline(pps);
            return true;
        }

        tok = pps.next_pp_token(true);
        if (tok.type != IDENTIFIER) {
            throw pp_exception("Expected an identifier", tok);
        }

        auto it = pps.macros.find(tok.str);
        if (it != pps.macros.end()) {
            pps.macros.erase(it);
        }

        tok = pps.next_pp_token(true);
        while (tok.type != NEW_LINE) {
            tok = pps.next_pp_token(true);
        }
        return true;
    } else if(tok.str == "line") {
        if (!current_group.is_enabled) {
            pp_eat_until_newiline(pps);
            return true;
        }

        tok = pps.next_pp_token(true);
        // TODO: #line

        while (tok.type != NEW_LINE) {
            tok = pps.next_pp_token(true);
        }
        return true;
    } else if(tok.str == "error") {
        if (!current_group.is_enabled) {
            pp_eat_until_newiline(pps);
            return true;
        }

        tok = pps.next_pp_token(true);
        std::string msg;
        while (tok.type != NEW_LINE) {
            msg += tok.str;
            tok = pps.next_pp_token(false);
        }
        throw pp_exception("#error: %s", tok, msg.c_str());
    } else if(tok.str == "pragma") {
        if (!current_group.is_enabled) {
            pp_eat_until_newiline(pps);
            return true;
        }

        tok = pps.next_pp_token(true);
        if (tok.type == IDENTIFIER && tok.str == "once") {
            pps.pragma_once();
        }
        while (tok.type != NEW_LINE) {
            tok = pps.next_pp_token(true);
        }
        return true;
    } else {
        tok = pps.next_pp_token(true);
        while (tok.type != NEW_LINE) {
            tok = pps.next_pp_token(true);
        }
        return true;
    }
}
