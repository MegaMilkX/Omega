#include "gpu_shader_program.hpp"

#include "gpu/gpu.hpp"


gpuShaderProgram::gpuShaderProgram(const char* vs, const char* fs) {
    init(vs, fs);
}

void gpuShaderProgram::init(const char* vs, const char* fs) {
    vid = glCreateShader(GL_VERTEX_SHADER);
    fid = glCreateShader(GL_FRAGMENT_SHADER);
    progid = glCreateProgram();
    glxShaderSource(vid, vs);
    glxShaderSource(fid, fs);
    if (!glxCompileShader(vid)) {}
    if (!glxCompileShader(fid)) {}
    glAttachShader(progid, vid);
    glAttachShader(progid, fid);

    // Attributes
    {/*
        GLint count;
        glGetProgramiv(progid, GL_ACTIVE_ATTRIBUTES, &count);
        for(int i = 0; i < count; ++i) {
            const GLsizei NAME_MAX_LEN = 64;
            GLchar name[NAME_MAX_LEN];
            GLsizei name_len;
            GLint size;
            GLenum type;
            glGetActiveAttrib(progid, (GLuint)i, NAME_MAX_LEN, &name_len, &size, &type, name);
            assert(name_len < NAME_MAX_LEN);
            std::string attrib_name(name, name + name_len);

            glBindAttribLocation(progid, i, attrib_name.c_str());
        }*/
    }

    // Outputs
    {
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
            return;
        }
    }

    // Uniforms/samplers
    {
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

            auto desc = VFMT::getAttribDescWithInputName(attrib_name.c_str());
            if (!desc) {
                continue;
            }
            attrib_table[desc->global_id] = attr_loc;
        }
    }

    // Uniform buffers
    {
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

            glUniformBlockBinding(progid, block_index, i);
        }
    }
}