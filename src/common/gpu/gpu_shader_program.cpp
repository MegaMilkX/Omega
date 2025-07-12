#include "gpu_shader_program.hpp"

#include "gpu/gpu.hpp"
#include "gpu/gpu_pipeline.hpp"


gpuShaderProgram::gpuShaderProgram(const char* vs, const char* fs) {
    setShaders(vs, fs);
    init();
}

bool gpuShaderProgram::compileAndAttach() {
    for (int i = 0; i < shaders.size(); ++i) {
        if (!glxCompileShader(shaders[i].id)) {
            return false;
        }
    }

    if (progid) {
        glDeleteProgram(progid);
    }
    progid = glCreateProgram();
    for (int i = 0; i < shaders.size(); ++i) {
        glAttachShader(progid, shaders[i].id);
    }

    return true;
}

void gpuShaderProgram::bindAttributeLocations() {
    GLint count;
    glGetProgramiv(progid, GL_ACTIVE_ATTRIBUTES, &count);
    for (int i = 0; i < count; ++i) {
        const GLsizei NAME_MAX_LEN = 64;
        GLchar name[NAME_MAX_LEN];
        GLsizei name_len;
        GLint size;
        GLenum type;
        glGetActiveAttrib(progid, (GLuint)i, NAME_MAX_LEN, &name_len, &size, &type, name);
        assert(name_len < NAME_MAX_LEN);
        std::string attrib_name(name, name + name_len);

        glBindAttribLocation(progid, i, attrib_name.c_str());
    }
}

void gpuShaderProgram::bindFragmentOutputLocations() {
    GLint count = 0;
    int name_len = 0;
    const int NAME_MAX_LEN = 64;
    char name[NAME_MAX_LEN];
    glGetProgramInterfaceiv(progid, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &count);
    for (int i = 0; i < count; ++i) {
        glGetProgramResourceName(progid, GL_PROGRAM_OUTPUT, i, NAME_MAX_LEN, &name_len, name);
        assert(name_len < NAME_MAX_LEN);
        std::string output_name(name, name + name_len);
        glBindFragDataLocation(progid, i, output_name.c_str());
        outputs.push_back(output_name);
    }
}

bool gpuShaderProgram::link() {
    GL_CHECK(glLinkProgram(progid));
    {
        GLint res = GL_FALSE;
        int infoLogLen;
        glGetProgramiv(progid, GL_LINK_STATUS, &res);
        glGetProgramiv(progid, GL_INFO_LOG_LENGTH, &infoLogLen);
        if (infoLogLen > 1)
        {
            std::vector<char> errMsg(infoLogLen + 1);
            glGetProgramInfoLog(progid, infoLogLen, NULL, &errMsg[0]);
            LOG_ERR("GLSL link: " << &errMsg[0]);
        }
        if (res != GL_TRUE) {
            LOG_ERR("Shader program failed to link");
            return false;
        }
    }
    return true;
}

void gpuShaderProgram::setSamplerIndices() {
    glUseProgram(progid);
    GLint count = 0;
    glGetProgramiv(progid, GL_ACTIVE_UNIFORMS, &count);
    int sampler_index = 0;
    for (int i = 0; i < count; ++i) {
        const GLsizei bufSize = 32;
        GLchar name[bufSize];
        GLsizei name_len;
        GLint size;
        GLenum type;
        glGetActiveUniform(progid, (GLuint)i, bufSize, &name_len, &size, &type, name);
        std::string uniform_name(name, name + name_len);
        if (type == GL_SAMPLER_2D_ARB) {
            sampler_indices[uniform_name] = sampler_index;
            sampler_names.push_back(uniform_name);

            GLint loc = glGetUniformLocation(progid, uniform_name.c_str());
            glUniform1i(loc, sampler_index++);
        } else if(type == GL_SAMPLER_BUFFER) {
            // TODO: Separate map for buffer samplers?
            sampler_indices[uniform_name] = sampler_index;
            sampler_names.push_back(uniform_name);
            GLint loc = glGetUniformLocation(progid, uniform_name.c_str());
            glUniform1i(loc, sampler_index++);
        } else if(type == GL_SAMPLER_CUBE_ARB) {
            sampler_indices[uniform_name] = sampler_index;
            sampler_names.push_back(uniform_name);

            GLint loc = glGetUniformLocation(progid, uniform_name.c_str());
            glUniform1i(loc, sampler_index++);
        } else if(type == GL_SAMPLER_CUBE_SHADOW) {
            sampler_indices[uniform_name] = sampler_index;
            sampler_names.push_back(uniform_name);

            GLint loc = glGetUniformLocation(progid, uniform_name.c_str());
            glUniform1i(loc, sampler_index++);
        }
    }
    sampler_count = sampler_index;
}

void gpuShaderProgram::getVertexAttributes() {
    LOG_DBG("Shader vertex attributes:");
    GLint count = 0;
    glGetProgramiv(progid, GL_ACTIVE_ATTRIBUTES, &count);
    for (int i = 0; i < count; ++i) {
        const GLsizei bufSize = 32;
        GLchar name[bufSize];
        GLsizei name_len;
        GLint size;
        GLenum type;
        glGetActiveAttrib(progid, (GLuint)i, bufSize, &name_len, &size, &type, name);
        std::string attrib_name(name, name + name_len);
        GLint attr_loc = glGetAttribLocation(progid, attrib_name.c_str());

        auto desc = VFMT::getAttribDescWithInputName(attrib_name.c_str());
        if (!desc) {
            LOG_ERR("Unknown attribute used in shader: " << attrib_name);
            continue;
        }
        LOG_DBG("\t" << attr_loc << ": " << desc->name);
        attrib_table[desc->global_id] = attr_loc;
    }
}

void gpuShaderProgram::setUniformBlockBindings() {
    /*{
        GLuint block_index = glGetUniformBlockIndex(progid, "ubCommon");
        if (block_index != GL_INVALID_INDEX) {
            glUniformBlockBinding(progid, block_index, 0);
        }
        block_index = glGetUniformBlockIndex(progid, "ubModel");
        if (block_index != GL_INVALID_INDEX) {
            glUniformBlockBinding(progid, block_index, 1);
        }
    }*/

    for (int i = 0; i < gpuGetPipeline()->uniformBufferCount(); ++i) {
        auto ub = gpuGetPipeline()->getUniformBuffer(i);

        GLuint block_index = glGetUniformBlockIndex(progid, ub->getName());
        if (block_index == GL_INVALID_INDEX) {
            //LOG_WARN("unsupported uniform buffer found");
            continue;
        }
        int uniform_count = ub->uniformCount();
        std::vector<const char*> names;
        std::vector<GLuint> indices;
        std::vector<GLint> offsets;
        names.resize(uniform_count);
        indices.resize(uniform_count);
        offsets.resize(uniform_count);
        for (int j = 0; j < uniform_count; ++j) {
            const char* name = ub->getUniformName(j);
            names[j] = name;
        }
        glGetUniformIndices(progid, uniform_count, names.data(), indices.data());
        glGetActiveUniformsiv(progid, uniform_count, indices.data(), GL_UNIFORM_OFFSET, offsets.data());

        for (int j = 0; j < uniform_count; ++j) {
            if (indices[j] == GL_INVALID_INDEX) {
                LOG_ERR("Uniform buffer '" << ub->getName() << "' member '" << names[j] << "' not found");
                assert(false);
                // TODO: Fail
            }
        }
        for (int j = 0; j < uniform_count; ++j) {
            if (offsets[j] != ub->getUniformByteOffset(j)) {
                LOG_ERR("Uniform buffer '" << ub->getName() << "' member '" << names[j] << "' offset mismatch: expected " << ub->getUniformByteOffset(j) << ", got " << offsets[j]);
                assert(false);
                // TODO: Fail
            }
        }

        glUniformBlockBinding(progid, block_index, ub->id);
    }
}

static UNIFORM_TYPE glTypeToUniformType(GLenum t) {
    switch(t) {
    case GL_BOOL:
        return UNIFORM_BOOL;
    case GL_INT:
        return UNIFORM_INT;
    case GL_UNSIGNED_INT:
        return UNIFORM_UINT;
    case GL_FLOAT:
        return UNIFORM_FLOAT;
    case GL_DOUBLE:
        return UNIFORM_DOUBLE;
    case GL_BOOL_VEC2:
        return UNIFORM_BVEC2;
    case GL_BOOL_VEC3:
        return UNIFORM_BVEC3;
    case GL_BOOL_VEC4:
        return UNIFORM_BVEC4;
    case GL_INT_VEC2:
        return UNIFORM_IVEC2;
    case GL_INT_VEC3:
        return UNIFORM_IVEC3;
    case GL_INT_VEC4:
        return UNIFORM_IVEC4;
    case GL_UNSIGNED_INT_VEC2:
        return UNIFORM_UVEC2;
    case GL_UNSIGNED_INT_VEC3:
        return UNIFORM_UVEC3;
    case GL_UNSIGNED_INT_VEC4:
        return UNIFORM_UVEC4;
    case GL_FLOAT_VEC2:
        return UNIFORM_VEC2;
    case GL_FLOAT_VEC3:
        return UNIFORM_VEC3;
    case GL_FLOAT_VEC4:
        return UNIFORM_VEC4;
    case GL_DOUBLE_VEC2:
        return UNIFORM_DVEC2;
    case GL_DOUBLE_VEC3:
        return UNIFORM_DVEC3;
    case GL_DOUBLE_VEC4:
        return UNIFORM_DVEC4;
    case GL_FLOAT_MAT2:
        return UNIFORM_MAT2;
    case GL_FLOAT_MAT2x3:
        return UNIFORM_MAT2X3;
    case GL_FLOAT_MAT2x4:
        return UNIFORM_MAT2X4;
    case GL_FLOAT_MAT3:
        return UNIFORM_MAT3;
    case GL_FLOAT_MAT3x2:
        return UNIFORM_MAT3X2;
    case GL_FLOAT_MAT3x4:
        return UNIFORM_MAT3X4;
    case GL_FLOAT_MAT4:
        return UNIFORM_MAT4;
    case GL_FLOAT_MAT4x2:
        return UNIFORM_MAT4X2;
    case GL_FLOAT_MAT4x3:
        return UNIFORM_MAT4X3;
    default:
        assert(false);
        return UNIFORM_UNKNOWN;
    }
}

void gpuShaderProgram::enumerateUniforms() {
    // Uniform block fields
    {
        uniform_blocks.clear();

        GLint count = 0;
        GL_CHECK(glGetProgramiv(progid, GL_ACTIVE_UNIFORM_BLOCKS, &count));
        LOG(count << " active uniform blocks");
        for (GLint i = 0; i < count; ++i) {
            const int BUF_SIZE = 256;
            char name[BUF_SIZE];
            int namelen = 0;
            GLint block_size = 0;
            GLint field_count = 0;
            glGetActiveUniformBlockName(progid, i, BUF_SIZE, &namelen, name);
            
            glGetActiveUniformBlockiv(progid, i, GL_UNIFORM_BLOCK_DATA_SIZE, &block_size);
            glGetActiveUniformBlockiv(progid, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &field_count);

            std::vector<GLint> field_indices(field_count);
            glGetActiveUniformBlockiv(progid, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, field_indices.data());

            bool match_failed = false;
            auto desc = gpuGetPipeline()->getUniformBufferDesc(name);
            if (desc == nullptr) {
                desc = gpuGetPipeline()->createUniformBufferDesc(name);
                for (GLint j = 0; j < field_count; ++j) {
                    char name[BUF_SIZE];
                    int namelen = 0;
                    GLint size = 0;
                    GLenum type = 0;
                    glGetActiveUniform(progid, field_indices[j], BUF_SIZE, &namelen, &size, &type, name);
                    desc->define(name, glTypeToUniformType(type));
                }
                desc->compile();
            } else {
                std::vector<GLint> offsets;
                offsets.resize(field_count);
                glGetActiveUniformsiv(progid, field_count, (const GLuint*)field_indices.data(), GL_UNIFORM_OFFSET, offsets.data());
                for (GLint j = 0; j < field_count; ++j) {
                    char name[BUF_SIZE];
                    int namelen = 0;
                    GLint size = 0;
                    GLenum type = 0;
                    
                    glGetActiveUniform(progid, field_indices[j], BUF_SIZE, &namelen, &size, &type, name);
                    
                    int uniform_id = desc->getUniform(name);
                    if (uniform_id < 0) {
                        match_failed = true;
                        LOG_ERR("No '" << name << "' field in uniform buffer desc '" << desc->getName() << "'");
                        assert(false);
                        break;
                    }

                    if (offsets[j] != desc->getUniformByteOffset(uniform_id)) {
                        match_failed = true;
                        LOG_ERR("Offset mismatch for '" << name << "' field of '" << desc->getName() << "' uniform buffer");
                        assert(false);
                        break;
                    }

                    // TODO: Check sizes?
                }
            }

            if (match_failed) {
                LOG_ERR("Failed to match uniform buffer desc: " << desc->getName());
                continue;
            }

            glUniformBlockBinding(progid, i, desc->id);

            uniform_blocks.push_back(desc);

            // Sanity check and logging
            GLint binding_point = 0;
            glGetActiveUniformBlockiv(progid, i, GL_UNIFORM_BLOCK_BINDING, &binding_point);

            LOG("\t" << name << ", size: " << block_size << ", binding: " << binding_point << ", field_count: " << field_count);
            for (GLint j = 0; j < field_count; ++j) {
                char name[BUF_SIZE];
                int namelen = 0;
                GLint size = 0;
                GLenum type = 0;
                glGetActiveUniform(progid, field_indices[j], BUF_SIZE, &namelen, &size, &type, name);
                LOG("\t\t" << name);
            }
        }
    }

    // Uniforms
    {
        uniforms.clear();

        GLint count = 0;
        GL_CHECK(glGetProgramiv(progid, GL_ACTIVE_UNIFORMS, &count));
        LOG(count << " active uniforms: ");
        for (GLint i = 0; i < count; ++i) {
            const int BUF_SIZE = 256;
            char namebuf[BUF_SIZE];
            int namelen = 0;
            GLint size = 0;
            GLenum type = 0;
            glGetActiveUniform(progid, i, BUF_SIZE, &namelen, &size, &type, namebuf);
            GLuint index = i;
            GLint ublock_index = -1;
            glGetActiveUniformsiv(progid, 1, &index, GL_UNIFORM_BLOCK_INDEX, &ublock_index);
            if (ublock_index >= 0) {
                continue;
            }
            LOG("\t" << namebuf << ", ubi: " << ublock_index);

            int location = glGetUniformLocation(progid, namebuf);
            if (location < 0) {
                assert(false);
            }

            uniforms.push_back(
                UNIFORM_INFO{
                    .name = namebuf,
                    .location = location,
                    .type = type
                }
            );
        }
    }
}

void gpuShaderProgram::clearShaders() {
    for (int i = 0; i < shaders.size(); ++i) {
        glDeleteShader(shaders[i].id);
    }
    shaders.clear();
}

void gpuShaderProgram::setShaders(const char* vs, const char* fs) {
    addShader(SHADER_VERTEX, vs);
    addShader(SHADER_FRAGMENT, fs);
}
void gpuShaderProgram::addShader(SHADER_TYPE type, const char* source) {
    GLenum gltype = 0;
    switch (type) {
    case SHADER_VERTEX:
        gltype = GL_VERTEX_SHADER;
        break;
    case SHADER_FRAGMENT:
        gltype = GL_FRAGMENT_SHADER;
        break;
    case SHADER_GEOMETRY:
        gltype = GL_GEOMETRY_SHADER;
        break;
    }
    
    GLuint id = glCreateShader(gltype);
    glxShaderSource(id, source);

    shaders.push_back(SHADER{ type, id });
}

void gpuShaderProgram::init() {
    compileAndAttach();

    //bindAttributeLocations();
    bindFragmentOutputLocations();

    if (!link()) {
        return;
    }

    setSamplerIndices();
    getVertexAttributes();
    //setUniformBlockBindings();
    enumerateUniforms();
}

void gpuShaderProgram::initForLightmapSampling() {
    compileAndAttach();

    glBindFragDataLocation(progid, 0, "outAlbedo");

    if (!link()) {
        return;
    }

    // Samplers
    {
        GLint loc = glGetUniformLocation(progid, "texAlbedo");
        if (loc) {
            sampler_indices["texAlbedo"] = 0;
            sampler_names.push_back("texAlbedo");
            glUniform1i(loc, 0);
        }
        loc = glGetUniformLocation(progid, "texLightmap");
        if (loc) {
            sampler_indices["texLightmap"] = 1;
            sampler_names.push_back("texLightmap");
            glUniform1i(loc, 1);
        }
        loc = glGetUniformLocation(progid, "texEmission");
        if (loc) {
            sampler_indices["texEmission"] = 2;
            sampler_names.push_back("texEmission");
            glUniform1i(loc, 2);
        }
        loc = glGetUniformLocation(progid, "texAmbientOcclusion");
        if (loc) {
            sampler_indices["texAmbientOcclusion"] = 3;
            sampler_names.push_back("texAmbientOcclusion");
            glUniform1i(loc, 3);
        }
    }

    getVertexAttributes();
    //setUniformBlockBindings();
    enumerateUniforms();
}

int gpuShaderProgram::uniformCount() {
    return (int)uniforms.size();
}
int gpuShaderProgram::getUniformIndex(const std::string& name) const {
    for (int i = 0; i < uniforms.size(); ++i) {
        if (uniforms[i].name == name) {
            return i;
        }
    }
    return -1;
}
const UNIFORM_INFO& gpuShaderProgram::getUniformInfo(int i) const {
    return uniforms[i];
}
UNIFORM_INFO& gpuShaderProgram::getUniformInfo(int i) {
    return uniforms[i];
}

int gpuShaderProgram::uniformBlockCount() {
    return (int)uniform_blocks.size();
}
const gpuUniformBufferDesc* gpuShaderProgram::getUniformBlockDesc(int i) const {
    return uniform_blocks[i];
}

GLint gpuShaderProgram::getUniformLocation(const char* name) const {
    return glGetUniformLocation(progid, name);
}
bool gpuShaderProgram::setUniform1i(const char* name, int i) {
    GLint u = getUniformLocation(name);
    if (u == -1) {
        return false;
    }
    GL_CHECK(glUniform1i(u, i));
    return true;
}
bool gpuShaderProgram::setUniform1f(const char* name, float f) {
    GLint u = getUniformLocation(name);
    if (u == -1) {
        return false;
    }
    GL_CHECK(glUniform1f(u, f));
    return true;
}
bool gpuShaderProgram::setUniform4f(const char* name, const gfxm::vec4& f4) {
    GLint u = getUniformLocation(name);
    if (u == -1) {
        return false;
    }
    GL_CHECK(glUniform4fv(u, 1, (float*)&f4));
    return true;
}
bool gpuShaderProgram::setUniform3f(const char* name, const gfxm::vec3& f3) {
    GLint u = getUniformLocation(name);
    if (u == -1) {
        return false;
    }
    GL_CHECK(glUniform3fv(u, 1, (float*)&f3));
    return true;
}
bool gpuShaderProgram::setUniformMatrix4(const char* name, const gfxm::mat4& m4) {
    GLint u = getUniformLocation(name);
    if (u == -1) {
        return false;
    }
    GL_CHECK(glUniformMatrix4fv(u, 1, GL_FALSE, (float*)&m4));
    return true;
}


static bool loadProgramText(const char* fpath, std::string& out) {
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

Handle<gpuShaderProgram> createProgram(const GLX_PP_CONTEXT& ctx, const char* source, size_t len) {
    struct PART {
        SHADER_TYPE type;
        size_t from, to;
        std::string preprocessed;
    };
        
    // TODO: Support multiple shaders of the same type
    //std::map<TYPE, PART> parts;
    std::vector<PART> parts;
    PART part = { SHADER_UNKNOWN, 0, 0 };
    for (int i = 0; i < len; ++i) {
        char ch = source[i];
        if (isspace(ch)) {
            continue;
        }
        if (ch == '#') {
            const char* tok = source + i;
            int tok_len = 0;
            for (int j = i; j < len; ++j) {
                ch = source[j];
                if (isspace(ch)) {
                    break;
                }
                tok_len++;
            }
            if (strncmp("#skip", tok, tok_len) == 0) {
                if (part.type != SHADER_UNKNOWN) {
                    part.to = i;
                    parts.push_back(part);
                }
                part.type = SHADER_UNKNOWN;
            } else if (strncmp("#vertex", tok, tok_len) == 0) {
                if (part.type != SHADER_UNKNOWN) {
                    part.to = i;
                    parts.push_back(part);
                }
                part.type = SHADER_VERTEX;
            } else if(strncmp("#fragment", tok, tok_len) == 0) {
                if (part.type != SHADER_UNKNOWN) {
                    part.to = i;
                    parts.push_back(part);
                }
                part.type = SHADER_FRAGMENT;
            } else if(strncmp("#geometry", tok, tok_len) == 0) {
                if (part.type != SHADER_UNKNOWN) {
                    part.to = i;
                    parts.push_back(part);
                }
                part.type = SHADER_GEOMETRY;
            } else {
                continue;
            }
            i += tok_len;
            for (; i < len; ++i) {
                ch = source[i];
                if (ch == '\n') {
                    ++i;
                    break;
                }
            }
            part.from = i;
        }
    }
    if (part.type != SHADER_UNKNOWN) {
        part.to = len;
        parts.push_back(part);
    }

    GLX_PP_CONTEXT pp_ctx = { 0 };
    std::vector<const char*> paths;
    paths.insert(paths.end(), ctx.include_paths, ctx.include_paths + ctx.n_include_paths);
    paths.push_back("./core/shaders");
    paths.push_back("./shaders");
    pp_ctx.include_paths = paths.data();
    pp_ctx.n_include_paths = paths.size();

    for (int i = 0; i < parts.size(); ++i) {
        bool result = glxPreprocessShaderIncludes(
            &pp_ctx,
            source + parts[i].from,
            parts[i].to - parts[i].from,
            parts[i].preprocessed
        );
        if (!result) {
            LOG_ERR("Failed to preprocess shader include directives");
            return Handle<gpuShaderProgram>();
        }
    }

    Handle<gpuShaderProgram> handle = HANDLE_MGR<gpuShaderProgram>::acquire();
    gpuShaderProgram* prog = HANDLE_MGR<gpuShaderProgram>::deref(handle);

    for (int i = 0; i < parts.size(); ++i) {
        prog->addShader(parts[i].type, parts[i].preprocessed.c_str());
    }

    return handle;
}
Handle<gpuShaderProgram> createProgram(const char* filepath, const char* source, size_t len) {
    LOG_DBG("Creating shader program: " << filepath);

    std::filesystem::path file_dir = filepath;
    file_dir = file_dir.parent_path();
    std::string str_file_dir = file_dir.string();

    GLX_PP_CONTEXT pp_ctx = { 0 };
    const char* paths[] = {
        str_file_dir.c_str()
    };
    pp_ctx.include_paths = paths;
    pp_ctx.n_include_paths = sizeof(paths) / sizeof(paths[0]);

    return createProgram(pp_ctx, source, len);
}


#include "resource/resource.hpp"

RHSHARED<gpuShaderProgram> loadShaderProgram(const char* path) {
    RHSHARED<gpuShaderProgram> prog = resFind<gpuShaderProgram>(path);
    if (prog.isValid()) {
        return prog;
    }

    std::string src;
    if (!loadProgramText(path, src)) {
        return RHSHARED<gpuShaderProgram>();
    }

    Handle<gpuShaderProgram> hprog = createProgram(path, src.c_str(), src.length());
    prog = RHSHARED<gpuShaderProgram>(hprog);
    if (!prog.isValid()) {
        return RHSHARED<gpuShaderProgram>();
    }

    prog->init();
    resStore(path, prog);
    return prog;
}

RHSHARED<gpuShaderProgram> loadShaderProgramForLightmapSampling(const char* path) {
    RHSHARED<gpuShaderProgram> prog = resFind<gpuShaderProgram>(path);
    if (prog.isValid()) {
        return prog;
    }

    std::string src;
    if (!loadProgramText(path, src)) {
        return RHSHARED<gpuShaderProgram>();
    }

    Handle<gpuShaderProgram> hprog = createProgram(path, src.c_str(), src.length());
    prog = RHSHARED<gpuShaderProgram>(hprog);
    if (!prog.isValid()) {
        return RHSHARED<gpuShaderProgram>();
    }

    prog->initForLightmapSampling();
    resStore(path, prog);
    return prog;
}
