#include "gpu/util_shader.hpp"
#include "log/log.hpp"
#include "filesystem/filesystem.hpp"

static enum SHADER_TYPE_ { SHADER_UNKNOWN, SHADER_VERTEX, SHADER_FRAGMENT };
static struct SHADER_PART_ {
    SHADER_TYPE_ type;
    const char* begin;
    const char* end;
};

static GLenum glxShaderTypeToGlEnum(SHADER_TYPE_ type) {
    switch (type)
    {
    case SHADER_VERTEX:
        return GL_VERTEX_SHADER;
    case SHADER_FRAGMENT:
        return GL_FRAGMENT_SHADER;
    default:
        return 0;
    }
}

static void glxShaderSource(GLuint shader, const char* string, int len) {
    glShaderSource(shader, 1, &string, len == 0 ? 0 : &len);
}
static bool glxCompileShader(GLuint shader) {
    glCompileShader(shader);
    GLint res = GL_FALSE;
    int infoLogLen;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
    if(infoLogLen > 1)
    {
        std::vector<char> errMsg(infoLogLen + 1);
        glGetShaderInfoLog(shader, infoLogLen, NULL, &errMsg[0]);
        LOG_ERR("GLSL compile: " << &errMsg[0]);
    }
    if(res == GL_FALSE)
        return false;
    return true;
}
static bool glxLinkProgram(GLuint progid) {
    GL_CHECK(glLinkProgram(progid));
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
    return true;
}

static void glxSetUniform1i(GLuint progid, const char* name, int value) {
    GLint loc = glGetUniformLocation(progid, name);
    if (loc != -1) {
        glUniform1i(loc, value);
    }
}

static void prepareShaderProgram(GLuint progid) {
    // Set fragment output locations
    {
        glBindFragDataLocation(progid, 0, "outAlbedo");
        glBindFragDataLocation(progid, 1, "outNormal");
        glBindFragDataLocation(progid, 2, "outWorldPos");
        glBindFragDataLocation(progid, 3, "outRoughness");
        glBindFragDataLocation(progid, 4, "outMetallic");
        glBindFragDataLocation(progid, 5, "outEmission");
        glBindFragDataLocation(progid, 6, "outLightness");
        glBindFragDataLocation(progid, 7, "outFinal");
        /*
        GLint count = 0;
        int name_len = 0;
        const int NAME_MAX_LEN = 64;
        char name[NAME_MAX_LEN];
        glGetProgramInterfaceiv(progid, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &count);
        for (int i = 0; i < count; ++i) {
            glGetProgramResourceName(progid, GL_PROGRAM_OUTPUT, i, NAME_MAX_LEN, &name_len, name);
            assert(name_len < NAME_MAX_LEN);
            std::string output_name(name, name + name_len);
            LOG("Fragment output " << output_name << ": " << i);
            // TODO: glBindFragDataLocation will have no effect until next linkage
            // but before linkage glGetProgramInterfaceiv() will not return any outputs
            glBindFragDataLocation(progid, i, output_name.c_str());
            // TODO:
        }*/
    }

    // Vertex attributes
    {
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
            LOG("Vertex attribute " << attrib_name << ": " << attr_loc);
            // TODO:
        }
    }

    // Uniform buffers
    {
        GLint numBlocks = 0;
        glGetProgramiv(progid, GL_ACTIVE_UNIFORM_BLOCKS, &numBlocks);
        for (int i = 0; i < numBlocks; ++i) {
            GLint nameLen = 0;
            glGetActiveUniformBlockiv(progid, i, GL_UNIFORM_BLOCK_NAME_LENGTH, &nameLen);
            std::string name;
            name.resize(nameLen);
            glGetActiveUniformBlockName(progid, i, nameLen, 0, &name[0]);
            GLint binding = 0;
            glGetActiveUniformBlockiv(progid, i, GL_UNIFORM_BLOCK_BINDING, &binding);
            GLint size = 0;
            glGetActiveUniformBlockiv(progid, i, GL_UNIFORM_BLOCK_DATA_SIZE, &size);
            LOG("Uniform buffer " << name << ", size: " << size << ", binding: " << binding);
            // TODO: Got uniform block name, can now:
            // 1. Check if name is known
            // 2. Check that all fields align with the cpp struct
            // 3. Set common uniform buffer bindings
            // 4. The other ones - just set sequentially
        }
    }

    // Samplers
    {
        glUseProgram(progid);
        glxSetUniform1i(progid, "texDiffuse", 0);
        glxSetUniform1i(progid, "texNormal", 1);
        glxSetUniform1i(progid, "texWorldPos", 2);
        glxSetUniform1i(progid, "texRoughness", 3);
        glxSetUniform1i(progid, "texMetallic", 4);
        glxSetUniform1i(progid, "texEmission", 5);
        glxSetUniform1i(progid, "texLightness", 6);
        glxSetUniform1i(progid, "texDepth", 7);
        glxSetUniform1i(progid, "cubemapEnvironment", 8);
        glxSetUniform1i(progid, "texBrdfLut", 9);
        glxSetUniform1i(progid, "cubemapIrradiance", 10);
        glxSetUniform1i(progid, "cubemapSpecular", 11);
        glxSetUniform1i(progid, "texHdri", 12);        
        glUseProgram(0);
    }

    {
        GLuint block_index = glGetUniformBlockIndex(progid, "ubCommon");
        if (block_index != GL_INVALID_INDEX) {
            glUniformBlockBinding(progid, block_index, 0);
        }
        block_index = glGetUniformBlockIndex(progid, "ubModel");
        if (block_index != GL_INVALID_INDEX) {
            glUniformBlockBinding(progid, block_index, 1);
        }
    }
}

static GLuint glxCreateShaderProgram(SHADER_PART_* parts, size_t count) {
    std::vector<GLuint> shaders;
    shaders.resize(count);
    for (int i = 0; i < count; ++i) {
        GLuint id = glCreateShader(glxShaderTypeToGlEnum(parts[i].type));
        shaders[i] = id;
        glxShaderSource(id, parts[i].begin, (size_t)(parts[i].end - parts[i].begin));
        if (!glxCompileShader(id)) {
            glDeleteShader(id);
            return 0;
        }
    }

    auto fnDeleteShaders = [&shaders, count]() {
        for (int i = 0; i < count; ++i) {
            glDeleteShader(shaders[i]);
        }
    };

    GLuint progid = glCreateProgram();
    for (int i = 0; i < count; ++i) {
        glAttachShader(progid, shaders[i]);
    }

    if (!glxLinkProgram(progid)) {
        glDeleteProgram(progid);
        fnDeleteShaders();
        return 0;
    }

    fnDeleteShaders();

    prepareShaderProgram(progid);
    return progid;
}

GLuint loadUtilShader(const char* filename) {
    std::string src;
    if (!fsSlurpTextFile(filename, src)) {
        LOG_ERR("Failed to open shader source file " << filename);
        return 0;
    }
    if (src.empty()) {
        LOG_ERR("Shader source empty");
        return 0;
    }

    std::vector<SHADER_PART_> parts;
    {
        const char* str = src.data();
        size_t len = src.size();

        SHADER_PART_ part = { SHADER_UNKNOWN, 0, 0 };
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
                    if (part.type != SHADER_UNKNOWN) {
                        part.end = str + i;
                        parts.push_back(part);
                    }
                    part.type = SHADER_VERTEX;
                } else if(strncmp("#fragment", tok, tok_len) == 0) {
                    if (part.type != SHADER_UNKNOWN) {
                        part.end = str + i;
                        parts.push_back(part);
                    }
                    part.type = SHADER_FRAGMENT;
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
                part.begin = str + i;
            }
        }
        if (part.type != SHADER_UNKNOWN) {
            part.end = str + len;
            parts.push_back(part);
        }
    }

    return glxCreateShaderProgram(parts.data(), parts.size());
}
