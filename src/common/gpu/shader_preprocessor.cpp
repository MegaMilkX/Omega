#include "shader_preprocessor.hpp"

#include <map>
#include <stack>
#include <memory>
#include <filesystem>
#include "log/log.hpp"
#include "filesystem/filesystem.hpp"

struct pp_file {
    const char* data;
    size_t len;
    int cur = 0;
    char ch;

    bool is_eof() const {
        return cur >= len;
    }
};

struct pp_state {
    std::map<std::string, std::unique_ptr<std::string>> file_cache;
    std::stack<pp_file> files;

    pp_state(const char* str, size_t length) {
        pp_file f;
        f.data = str;
        f.len = length;
        f.cur = 0;
        f.ch = f.data[f.cur];
        files.push(f);
    }

    bool include_file(const char* canonical_path) {
        //LOG_DBG("including '" << canonical_path << "'");
        auto it = file_cache.find(canonical_path);
        std::string* ptext = 0;
        if (it != file_cache.end()) {
            ptext = it->second.get();
        }
        else {
            std::string text;
            if (!fsSlurpTextFile(canonical_path, text)) {
                return false;
            }/*
            if (text.empty()) {
                return false;
            }*/
            if (text.back() != '\n') {
                text.push_back('\n');
            }
            ptext = new std::string(text);
            file_cache[canonical_path].reset(ptext);
        }

        files.push(pp_file());
        pp_file& f = files.top();
        f.data = ptext->data();
        f.len = ptext->size();
        f.cur = 0;
        f.ch = f.data[f.cur];
        return true;
    }

    pp_file* get_top_file() {
        if (files.empty()) {
            return 0;
        }
        while (files.top().is_eof()) {
            files.pop();
            if (files.empty()) {
                return 0;
            }
        }
        return &files.top();
    }

    bool get_line(std::string& out_line) {
        pp_file* f = get_top_file();
        if (!f) {
            return false;
        }

        const char* begin = f->data + f->cur;
        const char* end = begin;
        while (true) {
            if (f->is_eof()) {
                break;
            }
            ++end;
            f->ch = f->data[++f->cur];
            if (f->ch == '\n') {
                ++end;
                f->ch = f->data[++f->cur];
                break;
            }
        }
        out_line = std::string(begin, end);
        return true;
    }
};

struct parse_state {
    const char* data;
    size_t len;
    char ch;
    int cur;

    parse_state(const char* src, size_t length)
    : data(src), len(length) {
        cur = 0;
        ch = data[cur];
    }

    bool is_eol() const {
        return cur >= len;
    }
    void advance(int count) {
        int i = 0;
        while (cur < len && i < count) {
            ch = data[++cur];
            ++i;
        }
    }
    void skip_whitespace() {
        while (isspace(ch)) {
            ch = data[++cur];
        }
    }
    bool accept(char cha) {
        if (ch == cha) {
            advance(1);
            return true;
        }
        return false;
    }
    bool accept_str(const char* str_comp) {
        auto str_len = strlen(str_comp);
        auto leftover = len - cur;
        if (str_len > leftover) {
            return false;
        }
        if (strncmp(data + cur, str_comp, str_len) == 0) {
            advance(str_len);
            return true;
        }
        return false;
    }
};

bool glxPreprocessShaderIncludes(const char* path, const char* str, size_t len, std::string& result) {
    pp_state pps(str, len);

    std::string line;
    while (pps.get_line(line)) {
        parse_state ps(line.data(), line.size());

        ps.skip_whitespace();
        if (ps.accept('#')) {
            if (ps.accept_str("include")) {
                ps.skip_whitespace();
                
                if (ps.accept('\"')) {
                    const char* path_begin = ps.data + ps.cur;
                    const char* path_end = path_begin;
                    while (true) {
                        if (ps.ch == '\n') {
                            LOG_ERR("Encountered a newline in an include path");
                            return false;
                        }
                        if (ps.ch == '\"') {
                            path_end = ps.data + ps.cur;
                            break;
                        }
                        ps.advance(1);
                    }
                    std::string filepath(path_begin, path_end);
                    std::filesystem::path current_path = path;
                    std::filesystem::path incl_path = filepath;
                    if (incl_path.is_absolute()) {
                        if (!std::filesystem::exists(incl_path)) {
                            LOG_ERR("Can't include file " << incl_path.string());
                            return false;
                        }
                        incl_path = std::filesystem::canonical(incl_path);
                        if (!pps.include_file(incl_path.string().c_str())) {
                            LOG_ERR("Failed to include file " << incl_path.string());
                        }
                    } else {
                        current_path = std::filesystem::canonical(current_path);
                        std::filesystem::path dir_path = current_path.parent_path();
                        incl_path = dir_path / incl_path;
                        if (!std::filesystem::exists(incl_path)) {
                            LOG_ERR("Can't include file " << incl_path.string());
                            return false;
                        }
                        incl_path = std::filesystem::canonical(incl_path);
                        if (!pps.include_file(incl_path.string().c_str())) {
                            LOG_ERR("Failed to include file " << incl_path.string());
                        }
                    }

                } else {
                    LOG_ERR("#include directive must be followed by a file path in quotes");
                    return false;
                }
                continue;
            }
        }

        result += line;
    }

    return true;
}
