#pragma once

#include <ctime>
#include <filesystem>
#include "pp_exception.hpp"
#include "pp_macro_expansion.hpp"
#include "pp_eat.hpp"
#include "pp_util.hpp"
#define NOMINMAX
#include <Windows.h>


class preprocessor {
    HANDLE hConsole = INVALID_HANDLE_VALUE;

    bool keep_preprocessed_text = false;
    bool print_preprocessed_text = false;
    std::string preprocessed_text;
    int dbg_iter_count = -1;

    bool is_new_line = true;
    pp_state pp_state_;

    inline void dbg_report_token(PP_TOKEN& tok, bool relaxed = false) {
        if (keep_preprocessed_text) {
            preprocessed_text.insert(preprocessed_text.end(), tok.str.begin(), tok.str.end());
        }
        if (print_preprocessed_text) {
            if (relaxed) {
                SetConsoleTextAttribute(hConsole, 8);
                printf("%s", tok.str.c_str());
            } else {
                switch (tok.type) {
                case IDENTIFIER:
                    SetConsoleTextAttribute(hConsole, 0x17);
                    break;
                case STRING_LITERAL:
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    break;
                case CHARACTER_LITERAL:
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                    break;
                case PP_NUMBER:
                    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                    break;
                case PP_OP_OR_PUNC:
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    break;
                case WHITESPACE:
                    SetConsoleTextAttribute(hConsole, 8);
                    break;
                default:
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                };
                printf("%s", tok.str.c_str());
            }
        }
    }
public:
    preprocessor() {}
    preprocessor(bool ignore_missing_includes) {
        pp_state_.ignore_missing_includes = ignore_missing_includes;
    }
    preprocessor(const char* filename, bool keep_preprocessed_text, bool print_preprocessed_text) {
        init(filename, keep_preprocessed_text, print_preprocessed_text);
    }

    void add_include_directories(const std::vector<std::string>& dirs) {
        for (int i = 0; i < dirs.size(); ++i) {
            auto& path = dirs[i];
            std::experimental::filesystem::path stdpath = path;
            std::experimental::filesystem::path abspath =
                std::experimental::filesystem::canonical(stdpath);

            pp_state_.add_include_directory(abspath.string());
        }
    }

    void include_file(const std::string& path, bool angled_brackets = false) {
        pp_state_.include_file(path, angled_brackets);
    }
    bool include_file_current_dir(const std::string& spath) {
        std::experimental::filesystem::path path = spath;
        if (!path.is_absolute()) {
            path = std::experimental::filesystem::canonical(path);
        }
        return pp_state_.include_file_canonical(path);
    }

    const std::string& get_preprocessed_text() const {
        return preprocessed_text;
    }

    void predefine_macro(const char* macro, const char* replacement_list) {
        PP_MACRO m;
        m.has_va_args = false;
        m.is_function_like = false;
        std::vector<PP_TOKEN> tokens;
        parse_into_pp_tokens(replacement_list, strlen(replacement_list), tokens, 0);
        if (!tokens.empty() && tokens.front().type == WHITESPACE) {
            tokens.erase(tokens.begin());
        }
        if (!tokens.empty() && tokens.back().type == WHITESPACE) {
            tokens.pop_back();
        }
        m.replacement_list_ = tokens;
        pp_state_.macros[macro] = m;
    }

    bool init(const char* filename, bool keep_preprocessed_text, bool print_preprocessed_text) {
        this->keep_preprocessed_text = keep_preprocessed_text;
        this->print_preprocessed_text = print_preprocessed_text;

        std::experimental::filesystem::path fpath = filename;
        fpath = std::experimental::filesystem::canonical(fpath);
        if (!pp_state_.include_file_canonical(fpath.string())) {
            return false;
        }

        predefine_macro("__cplusplus", "201402L");
        const int MAXLEN = 80;
        char s[MAXLEN];
        time_t t = time(0);
        // Mmm dd yyyy
        strftime(s, MAXLEN, "%b %d %Y", localtime(&t));
        predefine_macro("__DATE__", make_string_literal(s).c_str());
        // hh:mm:ss
        strftime(s, MAXLEN, "%H:%M:%S", localtime(&t));
        predefine_macro("__TIME__", make_string_literal(s).c_str());
        // __FILE__ is handled intrinsically
        // __LINE__ is handled intrinsically
        predefine_macro("__STDC_HOSTED__", "0");

        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        return true;
    }

    PP_TOKEN next_token() {
        while (true) {
            ++dbg_iter_count;

            if (!pp_state_.has_files_to_read()) {
                PP_TOKEN t;
                t.type = END_OF_FILE;
                return t;
            }

            if (is_new_line) {
                if (pp_eat_directive(pp_state_)) {
                    continue;
                }
            }

            PP_TOKEN tok = pp_state_.next_pp_token(false);
            if (tok.type == END_OF_FILE) {
                if (pp_state_.get_current_group().type != PP_GROUP_NORMAL) {
                    throw pp_exception("Unexpected end of file (missing #endif)", 0, 0);
                }
                pp_state_.pop_file();
                continue;
            }
            if (tok.type == NEW_LINE) {
                dbg_report_token(tok);
                is_new_line = true;
                continue;
            }
            if(pp_state_.get_current_group().is_enabled) {
                if (tok.is_macro_expandable && tok.type == IDENTIFIER) {
                    std::vector<PP_TOKEN> list;
                    if (try_macro_expansion(pp_state_, tok, list)) {
                        pp_state_.insert_pp_tokens(list, true);
                        continue;
                    } else {
                        dbg_report_token(tok);
                        is_new_line = false;
                        return tok;
                    }
                } else if(tok.type == WHITESPACE) {
                    dbg_report_token(tok);
                    is_new_line = false;
                    continue;
                } else {
                    dbg_report_token(tok);
                    is_new_line = false;
                    return tok;
                }
            } else {
                while (tok.type != NEW_LINE) {
                    dbg_report_token(tok, true);
                    tok = pp_state_.next_pp_token(false);
                }
                dbg_report_token(tok, true);
                is_new_line = true;
                continue;
            }
        }
    }

};
