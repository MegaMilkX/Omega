#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include "resource_manager/loadable.hpp"
#include "gpu/types.hpp"


struct gpuCompiledShader {
    SHADER_TYPE type;
    uint32_t id; // GLuint
    gpuCompiledShader(SHADER_TYPE type);
    ~gpuCompiledShader();
    bool compile(const char* prefix, int32_t prefix_len, const char* source, int32_t len);
};

struct gpuCompiledShaderSet {
    std::vector<std::unique_ptr<gpuCompiledShader>> shaders;
};

class gpuShaderSet : public ILoadable {
    std::string dbg_name;

    struct SEGMENT {
        SHADER_TYPE type = SHADER_UNKNOWN;
        std::string raw;
        int first_line = 0;
        int version = 0;
    };
    std::vector<SEGMENT> segments;

    std::unordered_map<shader_flags_t, std::unique_ptr<gpuCompiledShaderSet>> compiled_sets;

    bool _loadSplitSegments(const char* source, size_t len);
public:
    gpuShaderSet() = default;
    gpuShaderSet(const gpuShaderSet& other) = delete;
    gpuShaderSet& operator=(const gpuShaderSet&) = delete;

    const gpuCompiledShaderSet* getCompiled(shader_flags_t flags);
    const std::string& dbgGetName() const { return dbg_name; }

    DEFINE_EXTENSIONS(e_glsl);
    bool load(byte_reader& reader);
};

