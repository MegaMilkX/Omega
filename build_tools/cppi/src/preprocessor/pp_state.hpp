#pragma once

#include <vector>
#include <memory>
#include <stack>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include "pp_types.hpp"
#include "pp_file.hpp"
#include "pp_parse.hpp"
#include "char_provider.hpp"


enum PP_GROUP_TYPE {
    PP_GROUP_NORMAL,
    PP_GROUP_IF,
    PP_GROUP_ELIF,
    PP_GROUP_ELSE
};
struct PP_GROUP {
    PP_GROUP_TYPE type;
    bool is_enabled;
    bool can_enable_else;
};

struct PP_FILE_WRAP {
    PP_FILE* file = 0;
    char_provider char_reader;
    std::stack<PP_GROUP> pp_group_stack;
};
struct PP_MACRO {
    bool is_function_like;
    bool has_va_args;
    std::vector<PP_TOKEN> params_;
    std::vector<PP_TOKEN> replacement_list_;
};

std::unordered_map<std::string, std::shared_ptr<PP_FILE>>& pp_get_file_cache();
PP_FILE* pp_find_cached_file(const std::string& canonical_path);
void pp_add_cached_file(PP_FILE* file);

class pp_state {
    std::vector<std::string> include_directories;
    std::unordered_set<std::string> pragma_once_paths;
    std::stack<std::shared_ptr<PP_FILE_WRAP>> file_stack;
    std::shared_ptr<PP_FILE_WRAP> latest_file;

    std::vector<PP_TOKEN> loaded_pp_tokens;
    int token_buffer_next = 0;

    PP_FILE* get_top_file() {
        if (file_stack.empty()) {
            return 0;
        }
        return file_stack.top().get()->file;
    }
    const PP_FILE* get_top_file() const {
        if (file_stack.empty()) {
            if (!latest_file) {
                return 0;
            }
            return latest_file.get()->file;
        }
        return file_stack.top().get()->file;
    }
    PP_FILE_WRAP* get_top_file_state() {
        if (file_stack.empty()) {
            return 0;
        }
        return file_stack.top().get();
    }
    const PP_FILE_WRAP* get_top_file_state() const {
        return file_stack.top().get();
    }

    PP_TOKEN& next_pp_token_() {
        if (token_buffer_next == loaded_pp_tokens.size()) {
            auto pp_file = get_top_file_state();
            assert(pp_file);
            PP_TOKEN tok;
            if (pp_file->char_reader.is_eof()) {
                tok.type = END_OF_FILE;
            } else {
                int adv = pp_parse_token(pp_file->char_reader, &tok);
                tok.is_macro_expandable = true;
            }
            tok.file = pp_file->file;

            loaded_pp_tokens.push_back(tok);
            PP_TOKEN& t = loaded_pp_tokens[token_buffer_next];
            ++token_buffer_next;
            return t;
        } else {
            PP_TOKEN& t = loaded_pp_tokens[token_buffer_next];
            ++token_buffer_next;
            return t;
        }
    }

    bool read_file(const std::string& path, std::vector<char>& data) {
        const std::string& fname = path;
        //const std::wstring wfname(path.begin(), path.end());
        //FILE* f = _wfopen(wfname.c_str(), L"rb");
        FILE* f = fopen(fname.c_str(), "rb");
        if (!f) {
            return false;
        }

        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (fsize == 0) {
            fclose(f);
            return true;
        }

        data.resize(fsize);
        fread(data.data(), fsize, 1, f);
        fclose(f);

        return true;
    }

public:
    bool ignore_missing_includes = false;
    std::unordered_map<std::string, PP_MACRO> macros;
    std::set<std::string> macro_expansion_stack;

    pp_state() {}

    void add_include_directory(const std::string& path) {
        include_directories.push_back(path);
    }
    bool include_file_canonical(const std::filesystem::path& abs_path) {
        if (!abs_path.is_absolute()) {
            assert(false);
            return false;
        }
        if (!abs_path.has_filename()) {
            assert(false);
            return false;
        }
        std::filesystem::path dir_path = abs_path.parent_path();

        //
        auto it = pragma_once_paths.find(abs_path.string());
        if (it != pragma_once_paths.end()) {
            // false means file wasn't found and will cause an exception
            // so in this case we just do nothing and report success
            
            // No need to link this file as a dependency,
            // since if it was pragma onced, it is already linked
            // as a dependency even if through multiple steps
            return true;
        }

        {
            PP_FILE* pp_file = pp_find_cached_file(abs_path.string());
            if (!pp_file) {
                pp_file = new PP_FILE;
                if (!read_file(abs_path.string(), pp_file->buf)) {
                    delete pp_file;
                    return false;
                }
                //std::chrono::system_clock::time_point tp
                std::filesystem::file_time_type file_tp
                    = std::filesystem::last_write_time(abs_path);
                std::chrono::system_clock::time_point tp(file_tp.time_since_epoch());
                pp_file->last_write_time = std::chrono::system_clock::to_time_t(tp);
                pp_file->file_name = abs_path.filename().string();
                pp_file->file_dir_abs = dir_path.string();
                pp_file->filename_canonical = abs_path.string();
                pp_file->parent = get_top_file();
                pp_add_cached_file(pp_file);
            }
            if (get_top_file()) {
                get_top_file()->dependencies.insert(pp_file);
                pp_file->dependants.insert(get_top_file());
            }

            std::shared_ptr<PP_FILE_WRAP> pp_file_wrap(new PP_FILE_WRAP);
            pp_file_wrap->char_reader = char_provider(pp_file->buf.data(), pp_file->buf.size());
            pp_file_wrap->pp_group_stack.push(PP_GROUP{ PP_GROUP_NORMAL, true });
            pp_file_wrap->file = pp_file;
            file_stack.push(pp_file_wrap);
        }
        return true;
    }
    bool include_file(const std::string& path, bool angled_brackets = false) {
        std::filesystem::path fpath = path;

        // Checking if it's a generated header include, so we can ignore it,
        // since it's not guaranteed to exist yet, and doesn't need to be parsed anyway
        if (!angled_brackets) {
            std::filesystem::path current_auto_name_path = get_file_name();
            current_auto_name_path.replace_extension(".auto" + current_auto_name_path.extension().string());
            if (current_auto_name_path.string() == fpath.filename().string()) {
                // Skipping the include
                return true;
            }
        }
        // ===========================

        if (fpath.is_absolute()) {
            // weird, but still need to handle
            if (!std::filesystem::exists(fpath)) {
                if (ignore_missing_includes) {
                    return true;
                }
                return false;
            }
            fpath = std::filesystem::canonical(fpath);
            if (!include_file_canonical(fpath.string())) {
                if (ignore_missing_includes) {
                    return true;
                }
                return false;
            }

            return true;
        }

        if (angled_brackets) {
            for (int i = 0; i < include_directories.size(); ++i) {
                std::filesystem::path fpath = include_directories[i];
                fpath /= path;
                fpath = std::filesystem::canonical(fpath);
                if (include_file_canonical(fpath.string())) {
                    return true;
                }
            }

            if (ignore_missing_includes) {
                return true;
            }
            return false;
        } else {
            auto cur_file = get_top_file();
            while (cur_file) {
                std::filesystem::path fpath = cur_file->file_dir_abs;
                fpath /= path;
                fpath = std::filesystem::canonical(fpath);
                if (include_file_canonical(fpath.string())) {
                    return true;
                }
                
                while (true) {
                    auto prev = cur_file;
                    cur_file = cur_file->parent;
                    if (!cur_file || cur_file->file_dir_abs != prev->file_dir_abs) {
                        break;
                    }
                }
            }

            for (int i = 0; i < include_directories.size(); ++i) {
                std::filesystem::path fpath = include_directories[i];
                fpath /= path;
                fpath = std::filesystem::canonical(fpath);
                if (include_file_canonical(fpath.string())) {
                    return true;
                }
            }

            if (ignore_missing_includes) {
                return true;
            }
            return false;
        }
    }

    // Remembers curent file's canonical file to be able to ignore it in further #includes
    void pragma_once() {
        auto cur_file = get_top_file();
        if (!cur_file) {
            return;
        }
        pragma_once_paths.insert(cur_file->filename_canonical);
    }

    const PP_GROUP& get_current_group() const {
        return get_top_file_state()->pp_group_stack.top();
    }
    void push_group(const PP_GROUP& grp) {
        get_top_file_state()->pp_group_stack.push(grp);
    }
    void pop_group() {
        get_top_file_state()->pp_group_stack.pop();
    }

    void pop_file() {
        if (!file_stack.empty()) {
            latest_file = file_stack.top();
        } else {
            assert(false);
        }
        file_stack.pop();
    }

    bool has_files_to_read() const { return !file_stack.empty(); }
    int get_line() const { return get_top_file_state()->char_reader.get_line(); }
    int get_col() const { return get_top_file_state()->char_reader.get_col(); }
    const std::string& get_file_name() const { return get_top_file()->file_name; }
    const PP_FILE* get_current_file() const { return get_top_file(); }
    int tokens_eaten_since_rewind() const { return token_buffer_next; }

    std::string next_characters_until_new_line() {
        assert(!file_stack.empty());
        int adv = 0;
        get_top_file_state()->char_reader.read_buf.clear();
        auto rw = get_top_file_state()->char_reader.get_rewind_point();

        std::string str;
        while (get_top_file_state()->char_reader.getch() != '\n') {
            if (get_top_file_state()->char_reader.getch() == '\0') {
                // NOTE: Last char in a file must be a new line
                assert(false);
                return str;
            }
            get_top_file_state()->char_reader.adv(1);
        }
        str = get_top_file_state()->char_reader.get_read_string(rw);
        return str;
    }
    PP_TOKEN    next_pp_token(bool skip_whitespace = false, bool skip_newline = false) {
        PP_TOKEN tok = next_pp_token_();
        while (
            (skip_whitespace && tok.type == WHITESPACE)
            || (skip_newline && tok.type == NEW_LINE)
            ) {
            tok = next_pp_token_();
        }
        return tok;
    }
    void set_pp_rewind_point() {
        loaded_pp_tokens.erase(loaded_pp_tokens.begin(), loaded_pp_tokens.begin() + token_buffer_next);
        token_buffer_next = 0;
    }
    void pp_rewind() {
        token_buffer_next = 0;
    }
    void insert_pp_tokens(const std::vector<PP_TOKEN>& tokens, bool mark_unexpandable = false) {
        loaded_pp_tokens.insert(loaded_pp_tokens.begin() + token_buffer_next, tokens.begin(), tokens.end());
        if (mark_unexpandable) {
            for (int i = token_buffer_next; i < loaded_pp_tokens.size(); ++i) {
                loaded_pp_tokens[i].is_macro_expandable = false;
            }
        }
    }
};
