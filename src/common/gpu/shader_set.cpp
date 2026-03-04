#include "shader_set.hpp"

#include "log/log.hpp"
#include "filesystem/filesystem.hpp"
#include "gpu/shader_preprocessor.hpp"
#include "platform/gl/glextutil.h"
#include "util/timer.hpp"


GLenum gpuShaderTypeToGLenum(SHADER_TYPE type) {
    switch (type) {
    case SHADER_VERTEX: return GL_VERTEX_SHADER;
    case SHADER_FRAGMENT: return GL_FRAGMENT_SHADER;
    case SHADER_GEOMETRY: return GL_GEOMETRY_SHADER;
    }
    return 0;
}

gpuCompiledShader::gpuCompiledShader(SHADER_TYPE type)
: type(type) {
    id = glCreateShader(gpuShaderTypeToGLenum(type));
}
gpuCompiledShader::~gpuCompiledShader() {
    glDeleteShader(id);
}
bool gpuCompiledShader::compile(const char* prefix, int32_t prefix_len, const char* source, int32_t len) {
    if(prefix) {
        const char* strings[2] = {
            prefix, source
        };
        int lengths[2] = {
            prefix_len, len
        };
        glShaderSource(id, 2, strings, lengths);
    } else {
        glShaderSource(id, 1, &source, &len);
    }

    glCompileShader(id);
    GLint res = GL_FALSE;
    int infoLogLen;
    glGetShaderiv(id, GL_COMPILE_STATUS, &res);
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLen);
    if(infoLogLen > 1)
    {
        std::vector<char> errMsg(infoLogLen + 1);
        glGetShaderInfoLog(id, infoLogLen, NULL, &errMsg[0]);
        LOG_ERR("GLSL compile:\n" << &errMsg[0]);
    }
    if(res == GL_FALSE) {
        LOG_ERR("Dump:\n" << std::string(source, source + len));
        return false;
    }
    return true;
}

std::string gpuShaderFlagsToPPDirectives(shader_flags_t flags) {
    std::string out;
    for (GPU_SHADER_DIRECTIVE dir = GPU_SHADER_DIRECTIVE_FIRST; dir != GPU_SHADER_DIRECTIVE_COUNT; ++dir) {
        if (flags & (uint64_t(1) << dir)) {
            out += MKSTR("#define " << gpuShaderDirectiveToString(dir) << " 1\n");
        }
    }
    return out;
}

const gpuCompiledShaderSet* gpuShaderSet::getCompiled(shader_flags_t flags) {
    // TODO: Mask out flags not supported by the set
    auto it = compiled_sets.find(flags);
    if (it != compiled_sets.end()) {
        return it->second.get();
    }
    
    gpuCompiledShaderSet* new_set = new gpuCompiledShaderSet;
    it = compiled_sets.insert(std::make_pair(flags, std::unique_ptr<gpuCompiledShaderSet>(new_set))).first;
    
    new_set->shaders.resize(segments.size());
    for (int i = 0; i < segments.size(); ++i) {
        const SEGMENT& seg = segments[i];
        auto& shader = new_set->shaders[i];
        shader.reset(new gpuCompiledShader(seg.type));
        std::string prefix;
        if(seg.version) {
            prefix = MKSTR("#version " << seg.version << "\n");
        }
        prefix += gpuShaderFlagsToPPDirectives(flags);
        LOG("Compiling " << gpuShaderTypeToString(seg.type) << " shader with prefix:\n" << (prefix.empty() ? "[none]" : prefix));
        timer timer_;
        timer_.start();
        shader->compile(prefix.c_str(), prefix.size(), seg.raw.c_str(), seg.raw.size());
        LOG_DBG("Compiled in " << timer_.stop() * 1000.f << "ms");
    }
    return it->second.get();
}

bool gpuShaderSet::_loadSplitSegments(const char* source, size_t len) {    
    struct TOKEN {
        const char* at;
        int len;

        operator bool() const {
            return len > 0;
        }
        bool operator==(const char* other) const {
            return strncmp(at, other, len) == 0;
        }
        std::string toString() const {
            return std::string(at, at + len);
        }
    };

    int nline = 0;

    auto nextLine = [source, len, &nline](int line_start, int &i)->void {
        for (;; ++i) {
            if (i >= len || source[i] == '\n') {
                ++nline;
                break;
            }
        }
    };

    auto eatWhitespace = [source, len](int &at)->bool {
        int begin = at;
        while (at < len && isspace(source[at]) && source[at] != '\n') {
            ++at;
            continue;
        }
        return begin < at;
    };

    auto eatIdentifier = [source, len](int &at)->TOKEN {
        int begin = at;
        const char* tok = source + at;
        
        if (!isalpha(source[at])) {
            return TOKEN{ tok, 0 };
        }
        ++at;

        while (at < len && isalnum(source[at])) {
            ++at;
            continue;
        }
        return TOKEN{ tok, at - begin };
    };
    auto eatVersionInt = [source, len](int &at, int& result)->bool {
        int begin = at;
        while (at < len && isdigit(source[at])) {
            ++at;
            continue;
        }
        result = 0;
        int base = 1;
        for (int i = 0; i < at - begin; ++i) {
            char c = source[at - 1 - i];
            int n = c - '0';
            result += n * base;
            base *= 10;
        }
        return begin < at;
    };

    SEGMENT part = { SHADER_UNKNOWN, "", 0, 0 };
    size_t part_begin = 0;

    auto commitPart = [this, source, &part, &part_begin](int at) {
        part.raw += std::string(source + part_begin, source + at);
        if (part.type != SHADER_UNKNOWN) {
            segments.push_back(part);
        }
    };
    auto appendPart = [source, &part, &part_begin](int at) {
        part.raw += std::string(source + part_begin, source + at);
    };    

    int i = 0;
    for (; i < len; ++i) {
        int line_start = i;

        eatWhitespace(i);
        if (source[i] == '#') {
            int part_end = i;
            ++i;
            eatWhitespace(i);
            TOKEN tok = eatIdentifier(i);
            if (tok == "vertex") {
                commitPart(part_end);
                part.type = SHADER_VERTEX;
                part.version = 0;
                part.raw.clear();
                nextLine(line_start, i);
                part_begin = i;
                part.first_line = nline;
            } else if (tok == "fragment") {
                commitPart(part_end);
                part.type = SHADER_FRAGMENT;
                part.version = 0;
                part.raw.clear();
                nextLine(line_start, i);
                part_begin = i;
                part.first_line = nline;
            } else if (tok == "geometry") {
                commitPart(part_end);
                part.type = SHADER_GEOMETRY;
                part.version = 0;
                part.raw.clear();
                nextLine(line_start, i);
                part_begin = i;
                part.first_line = nline;
            } else if (tok == "skip") {
                commitPart(part_end);
                part.type = SHADER_UNKNOWN;
                part.version = 0;
                part.raw.clear();
                nextLine(line_start, i);
                part_begin = i;
                part.first_line = nline;
            } else if (tok == "version") {
                appendPart(part_end);
                if (part.version != 0) {
                    LOG_ERR("Duplicate #version directive");
                    nextLine(line_start, i);
                    continue;
                }
                eatWhitespace(i);
                int ver = 0;
                if (!eatVersionInt(i, ver)) {
                    LOG_ERR("Invalid token after #version");
                    nextLine(line_start, i);
                    continue;
                }
                part.version = ver;
                nextLine(line_start, i);
                part_begin = i;
                part.raw += "\n"; // Keep line indices consistent
            } else {
                nextLine(line_start, i);
            }
            continue;
        }

        nextLine(line_start, i);
    }
    commitPart(len);

    GLX_PP_CONTEXT pp_ctx = { 0 };
    std::vector<const char*> paths;
    paths.push_back("./core/shaders");
    paths.push_back("./shaders");
    pp_ctx.include_paths = paths.data();
    pp_ctx.n_include_paths = paths.size();

    for (int i = 0; i < segments.size(); ++i) {
        SEGMENT& seg = segments[i];
        std::string preprocessed;
        bool result = glxPreprocessShaderIncludes(
            &pp_ctx,
            seg.raw.data(), seg.raw.size(),
            preprocessed, seg.first_line
        );

        if (!result) {
            LOG_ERR("Failed to preprocess shader include directives");
            return false;
        }
        seg.raw = std::move(preprocessed);
    }

    return true;
}

bool gpuShaderSet::load(byte_reader& reader) {
    auto view = reader.try_slurp();
    if (!view) {
        return false;
    }

    dbg_name = reader.filename_hint();
    return _loadSplitSegments((const char*)view.data, view.size);
}

