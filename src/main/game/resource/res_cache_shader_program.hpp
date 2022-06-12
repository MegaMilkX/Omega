#pragma once

#include "res_cache_interface.hpp"

#include <assert.h>
#include <string>
#include <map>
#include <memory>
#include <fstream>
#include "common/render/gpu_shader_program.hpp"


class resCacheShaderProgram : public resCacheInterface {
    std::map<std::string, std::unique_ptr<gpuShaderProgram>> programs;

    bool loadProgramText(const char* fpath, std::string& out) {
        std::ifstream f(fpath);
        if (!f) {
            assert(false);
            return false;
        }
        f.seekg(0, std::ios::end);
        size_t sz = f.tellg();
        
        out.resize(sz);
        
        f.seekg(0);
        f.read(&out[0], sz);

        return true;
    }

    gpuShaderProgram* createProgram(const char* str, size_t len) {
        enum TYPE { UNKNOWN, VERTEX, FRAGMENT };
        struct PART {
            TYPE type;
            size_t from, to;
        };
        
        // TODO: Support multiple shaders of the same type
        std::map<TYPE, PART> parts;
        PART part = { UNKNOWN, 0, 0 };
        for (int i = 0; i < len; ++i) {
            char ch = str[i];
            if (isspace(ch)) {
                continue;
            }
            if (ch == '#') {
                const char* tok = str + i;
                int tok_len = 0;
                for (int j = i; j < len; ++j) {
                    ch = str[j];
                    if (isspace(ch)) {
                        break;
                    }
                    tok_len++;
                }
                if (strncmp("#vertex", tok, tok_len) == 0) {
                    if (part.type != UNKNOWN) {
                        part.to = i;
                        parts[part.type] = part;
                    }
                    part.type = VERTEX;
                } else if(strncmp("#fragment", tok, tok_len) == 0) {
                    if (part.type != UNKNOWN) {
                        part.to = i;
                        parts[part.type] = part;
                    }
                    part.type = FRAGMENT;
                } else {
                    continue;
                }
                i += tok_len;
                for (; i < len; ++i) {
                    ch = str[i];
                    if (ch == '\n') {
                        ++i;
                        break;
                    }
                }
                part.from = i;
            }
        }
        if (part.type != UNKNOWN) {
            part.to = len;
            parts[part.type] = part;
        }

        auto& pt_vertex = parts[VERTEX];
        auto& pt_frag = parts[FRAGMENT];
        std::string str_vs(str + pt_vertex.from, str + pt_vertex.to);
        std::string str_fs(str + pt_frag.from, str + pt_frag.to);
        gpuShaderProgram* prog = new gpuShaderProgram(str_vs.c_str(), str_fs.c_str());
        /*for (int i = 0; i < parts.size(); ++i) {
            auto p = parts[i];
            LOG("Program part type " << p.type);
            LOG(std::string(str + p.from, str + p.to));
        }*/

        return prog;
    }

    gpuShaderProgram* loadProgramNew(const char* fpath) {
        std::string src;
        if (!loadProgramText(fpath, src)) {
            return 0;
        }

        return createProgram(src.c_str(), src.size());
    }

    gpuShaderProgram* loadProgram(const char* fpath) {
        auto it = programs.find(fpath);
        if (it == programs.end()) {
            auto ptr = loadProgramNew(fpath);
            it = programs.insert(std::make_pair(std::string(fpath), std::unique_ptr<gpuShaderProgram>(ptr))).first;
        }
        return it->second.get();
    }

public:
    void* get(const char* name) override {
        return loadProgram(name);
    }
    
};
