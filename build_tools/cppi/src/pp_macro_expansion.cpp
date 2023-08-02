#include "pp_macro_expansion.hpp"
#include "pp_exception.hpp"
#include "pp_util.hpp"


int try_macro_expansion(pp_state& pps, const PP_TOKEN& expanded_tok, std::vector<PP_TOKEN>& list, bool keep_defined_op_param) {
    if (keep_defined_op_param && expanded_tok.type == IDENTIFIER && expanded_tok.str == "defined") {
        //emit_warning("defined operator resulting from a macro expansion", &pps);

        pps.set_pp_rewind_point();
        int tokens_eaten = 0;
        list.push_back(expanded_tok);
        PP_TOKEN tok = pps.next_pp_token(true, false);
        if (tok.type == IDENTIFIER) {
            list.push_back(tok);
            return 2;
        } else if(tok.type == PP_OP_OR_PUNC && tok.str == "(") {
            list.push_back(tok);

            tok = pps.next_pp_token(true, false);
            if (tok.type != IDENTIFIER) {
                throw pp_exception("defined operator expected an identifier", tok);
            }
            list.push_back(tok);
            
            tok = pps.next_pp_token(true, false);
            if (tok.type != PP_OP_OR_PUNC || tok.str != ")") {
                throw pp_exception("defined operator expected a closing parenthesis", tok);
            }
            list.push_back(tok);

            return 4;
        } else {
            throw pp_exception("defined operator missing an operand", tok);
        }
    }
    if (expanded_tok.type == IDENTIFIER && expanded_tok.is_macro_expandable) {
        if (expanded_tok.str == "__LINE__") {
            PP_TOKEN t;
            t.is_macro_expandable = false;
            t.line = expanded_tok.line;
            t.col = expanded_tok.col;
            t.type = PP_NUMBER;
            int ln = pps.get_line();
            char buf[20];
            itoa(ln, buf, 10);
            t.str = buf;
            list.push_back(t);
            return 1;
        } else if(expanded_tok.str == "__FILE__") {
            PP_TOKEN t;
            t.is_macro_expandable = false;
            t.line = expanded_tok.line;
            t.col = expanded_tok.col;
            t.type = STRING_LITERAL;
            t.str = make_string_literal(pps.get_file_name().c_str());
            list.push_back(t);
            return 1;
        }
        auto it = pps.macros.find(expanded_tok.str);
        if (it == pps.macros.end()) {
            return 0;
        }
        auto it2 = pps.macro_expansion_stack.find(expanded_tok.str);
        if (it2 != pps.macro_expansion_stack.end()) {
            return 0;
        }
        PP_MACRO& m = it->second;
        if (!m.is_function_like) {
            // Object-like macro
            pps.macro_expansion_stack.insert(it->first);

            auto replacement_list_original = it->second.replacement_list_;
            std::vector<PP_TOKEN> replacement_list_;
            bool concatenate = false;
            for (int i = 0; i < replacement_list_original.size(); ++i) {
                auto& tk = replacement_list_original[i];
                if (tk.type == WHITESPACE) {
                    if (concatenate) {
                        concatenate = false;
                        continue;
                    }
                } else if(tk.type == PP_OP_OR_PUNC) {
                    if (tk.str == "##") {
                        concatenate = true;
                        if (replacement_list_.back().type == WHITESPACE) {
                            replacement_list_.pop_back();
                        }
                        replacement_list_.push_back(tk);
                        continue;
                    }
                }
                replacement_list_.push_back(tk);
            }
            for (int i = 0; i < replacement_list_.size(); ++i) {
                auto& tk = replacement_list_[i];
                if (tk.type == PP_OP_OR_PUNC && tk.str == "##" && tk.is_macro_expandable) {
                    // ## can't appear first or last so it's safe to look one place ahead or back
                    auto& tk_a = replacement_list_[i - 1];
                    auto& tk_b = replacement_list_[i + 1];

                    std::string concatenated;
                    concatenated = tk_a.str + tk_b.str;
                    std::vector<PP_TOKEN> tokens;
                    parse_into_pp_tokens(concatenated.c_str(), concatenated.size(), tokens, pps.get_current_file());
                    assert(tokens.size() > 0);
                    if (tokens.size() > 1) {
                        emit_warning("## results in an invalid preprocessing token (concatenated token reparsed into multiple tokens)", &pps);
                    }
                    for (int j = 0; j < tokens.size(); ++j) {
                        tokens[j].is_macro_expandable = false;
                        tokens[j].line = expanded_tok.line;
                        tokens[j].col = expanded_tok.col;
                    }
                    replacement_list_.erase(replacement_list_.begin() + i - 1);
                    replacement_list_.erase(replacement_list_.begin() + i - 1);
                    replacement_list_.erase(replacement_list_.begin() + i - 1);
                    --i;
                    replacement_list_.insert(replacement_list_.begin() + i, tokens.begin(), tokens.end());
                    i += tokens.size() - 1;
                    continue;
                }
            }
            pps.insert_pp_tokens(replacement_list_);
            int i = 0;
            while (i < replacement_list_.size()) {
                PP_TOKEN tok = pps.next_pp_token(false);
                std::vector<PP_TOKEN> sub_list;
                int tokens_eaten = try_macro_expansion(pps, tok, sub_list, keep_defined_op_param);
                if (tokens_eaten == 0) {
                    list.push_back(tok);
                    ++i;
                } else {
                    list.insert(list.end(), sub_list.begin(), sub_list.end());
                    i += tokens_eaten;
                }
            }
            pps.macro_expansion_stack.erase(it->first);
            return 1; // One token expanded
        } else {
            // Possible function-like macro
            // Now check if there's an argument list
            pps.set_pp_rewind_point();
            int tokens_replaced = 0; // Total tokens in this macro invocation
            PP_TOKEN tok = pps.next_pp_token(true, true);
            if (tok.type == PP_OP_OR_PUNC && tok.str == "(") {
                std::vector<std::vector<PP_TOKEN>> arg_list;
                int nesting_counter = 0;
                int arg_n = 0;
                arg_list.push_back(std::vector<PP_TOKEN>());
                tok = pps.next_pp_token(true, true);
                while (!(tok.str == ")" && nesting_counter == 0)) {
                    if (tok.type == END_OF_FILE) {
                        throw pp_exception("Function-like macro invocation missing a closing parenthesis", expanded_tok);
                    }
                    if (nesting_counter == 0 && tok.str == ",") {
                        if (arg_n < m.params_.size() - 1) {
                            arg_list.push_back(std::vector<PP_TOKEN>());
                            ++arg_n;
                            tok = pps.next_pp_token(true, true);
                            continue;
                        }
                    }
                    if (tok.type != NEW_LINE) {
                        if (arg_n < m.params_.size()) {
                            if (arg_list[arg_n].size() > 0 && arg_list[arg_n].back().type == WHITESPACE && tok.type == WHITESPACE) {
                                arg_list[arg_n].back().str += tok.str;
                            } else {
                                arg_list[arg_n].push_back(tok);
                            }
                        } else {
                            assert(false);
                        }
                    }
                    if (tok.str == "(") {
                        ++nesting_counter;
                    } else if(tok.str == ")") {
                        // can't reach here if nesting_counter is 0, so no need to check
                        --nesting_counter;
                    }
                    tok = pps.next_pp_token(false, true);
                }
                tokens_replaced = pps.tokens_eaten_since_rewind();
                for (int i = 0; i < arg_list.size(); ++i) {
                    auto& arg = arg_list[i];
                    if (arg.empty()) {
                        continue;
                    }
                    if (arg[arg.size() - 1].type == WHITESPACE) {
                        arg.pop_back();
                    }
                }
                for (int i = 0; i < arg_list.size(); ++i) {
                    auto& arg = arg_list[i];
                    if (arg.empty()) {
                        PP_TOKEN t;
                        t.is_macro_expandable = false;
                        t.line = expanded_tok.line;
                        t.col = expanded_tok.col;
                        t.type = PLACEMARKER;
                        t.str = "";
                        arg.push_back(t);
                        continue;
                    }
                }

                pps.macro_expansion_stack.insert(it->first);
                auto replacement_list_original = it->second.replacement_list_;
                auto& params = it->second.params_;
                bool concatenate = false;
                bool stringify = false;
                std::vector<PP_TOKEN> replacement_list_;
                for (int i = 0; i < replacement_list_original.size(); ++i) {
                    auto& tk = replacement_list_original[i];
                    if (tk.type == WHITESPACE) {
                        if (concatenate) {
                            concatenate = false;
                            continue;
                        }
                        if (stringify) {
                            continue;
                        }
                    }
                    if (tk.type == MACRO_PARAM) {
                        int param_idx = tk.param_idx;
                        if (param_idx != -1) {
                            // Is parameter
                            if (param_idx < arg_list.size()) {
                                auto arg_ = arg_list[param_idx];

                                if(m.params_[param_idx].is_macro_expandable) {
                                    std::vector<PP_TOKEN> expanded_arg;
                                    pps.insert_pp_tokens(arg_);
                                    int k = 0;
                                    while (k < arg_.size()) {
                                        PP_TOKEN tok = pps.next_pp_token(false);
                                        std::vector<PP_TOKEN> sub_list;
                                        int tokens_eaten = try_macro_expansion(pps, tok, sub_list, keep_defined_op_param);
                                        if (tokens_eaten == 0) {
                                            expanded_arg.push_back(tok);
                                            ++k;
                                        } else {
                                            expanded_arg.insert(expanded_arg.end(), sub_list.begin(), sub_list.end());
                                            k += tokens_eaten;
                                        }
                                    }
                                    arg_ = expanded_arg;
                                }
                                // Macro arguments are already expanded before being inserted in place of parameter placeholders
                                for (auto& a : arg_) {
                                    a.is_macro_expandable = false;
                                }

                                if (stringify) {
                                    std::string str = "\"";
                                    for (auto& t : arg_) {
                                        std::string s = t.str;
                                        if (t.type == WHITESPACE) {
                                            s = " ";
                                        } else if (t.type == STRING_LITERAL
                                            || t.type == CHARACTER_LITERAL
                                            || t.type == USER_DEFINED_STRING_LITERAL
                                            || t.type == USER_DEFINED_CHARACTER_LITERAL
                                        ) {
                                            for (int j = 0; j < s.size(); ++j) {
                                                if (s[j] == '\\' || s[j] == '"') {
                                                    s.insert(s.begin() + j, '\\');
                                                    ++j;
                                                }
                                            }
                                        }
                                        str += s;
                                    }
                                    str += "\"";
                                    PP_TOKEN t;
                                    t.type = STRING_LITERAL;
                                    t.str = str;
                                    replacement_list_.push_back(t);
                                    stringify = false;
                                } else {
                                    replacement_list_.insert(replacement_list_.end(), arg_.begin(), arg_.end());
                                }
                            }
                            continue;
                        }
                    }else if(tk.type == PP_OP_OR_PUNC) {
                        if (tk.str == "##") {
                            // Only removing whitespace now, concatenating later
                            concatenate = true;
                            if (replacement_list_.back().type == WHITESPACE) {
                                replacement_list_.pop_back();
                            }
                            replacement_list_.push_back(tk);
                            continue;
                        } else if(tk.str == "#") {
                            stringify = true;
                            continue;
                        }
                    }
                    replacement_list_.push_back(tk);
                }
                for (int i = 0; i < replacement_list_.size(); ++i) {
                    auto& tk = replacement_list_[i];
                    if (tk.type == PP_OP_OR_PUNC && tk.str == "##" && tk.is_macro_expandable) {
                        // ## can't appear first or last so it's safe to look one place ahead or back
                        auto& tk_a = replacement_list_[i - 1];
                        auto& tk_b = replacement_list_[i + 1];
                        if (tk_a.type == PLACEMARKER && tk_b.type == PLACEMARKER) {
                            // Only deleting two tokens
                            // one placemarker needs to stay for cases like P ## P ## P
                            replacement_list_.erase(replacement_list_.begin() + i);
                            replacement_list_.erase(replacement_list_.begin() + i);
                            --i;
                            continue;
                        } else if(tk_a.type == PLACEMARKER) {
                            replacement_list_.erase(replacement_list_.begin() + i - 1);
                            replacement_list_.erase(replacement_list_.begin() + i - 1);
                            --i;
                            continue;
                        } else if(tk_b.type == PLACEMARKER) {
                            replacement_list_.erase(replacement_list_.begin() + i);
                            replacement_list_.erase(replacement_list_.begin() + i);
                            --i;
                            continue;
                        } else {
                            std::string concatenated;
                            concatenated = tk_a.str + tk_b.str;
                            std::vector<PP_TOKEN> tokens;
                            parse_into_pp_tokens(concatenated.c_str(), concatenated.size(), tokens, pps.get_current_file());
                            assert(tokens.size() > 0);
                            if (tokens.size() > 1) {
                                emit_warning("## results in an invalid preprocessing token (concatenated token reparsed into multiple tokens)", &pps);
                            }
                            for (int j = 0; j < tokens.size(); ++j) {
                                tokens[j].is_macro_expandable = false;
                                tokens[j].line = tk.line;
                                tokens[j].col = tk.col;
                            }
                            replacement_list_.erase(replacement_list_.begin() + i - 1);
                            replacement_list_.erase(replacement_list_.begin() + i - 1);
                            replacement_list_.erase(replacement_list_.begin() + i - 1);
                            --i;
                            replacement_list_.insert(replacement_list_.begin() + i, tokens.begin(), tokens.end());
                            i += tokens.size() - 1;
                            continue;
                        }
                    }
                }
                for (int i = 0; i < replacement_list_.size(); ++i) {
                    auto& t = replacement_list_[i];
                    if (t.type == PLACEMARKER) {
                        replacement_list_.erase(replacement_list_.begin() + i);
                        --i;
                    }
                }
                pps.insert_pp_tokens(replacement_list_);
                int i = 0;
                while (i < replacement_list_.size()) {
                    PP_TOKEN tok = pps.next_pp_token(false);
                    std::vector<PP_TOKEN> sub_list;
                    int tokens_eaten = try_macro_expansion(pps, tok, sub_list, keep_defined_op_param);
                    if (tokens_eaten == 0) {
                        list.push_back(tok);
                        ++i;
                    } else {
                        list.insert(list.end(), sub_list.begin(), sub_list.end());
                        i += tokens_eaten;
                    }
                }
                pps.macro_expansion_stack.erase(it->first);
                return tokens_replaced + 1;
            } else {
                pps.pp_rewind();
                return 0; // 
            }
        }
    }

    return 0;
}