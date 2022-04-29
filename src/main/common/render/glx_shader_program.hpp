#ifndef GLX_SHADER_PROGRAM_HPP
#define GLX_SHADER_PROGRAM_HPP

#include <vector>
#include "common/log/log.hpp"
#include "platform/gl/glextutil.h"


inline void glxShaderSource(GLuint shader, const char* string, int len = 0) {
    glShaderSource(shader, 1, &string, len == 0 ? 0 : &len);
}
inline bool glxCompileShader(GLuint shader) {
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
#include "vertex_format.hpp"
inline void glxProgramBindVertexFormat(GLuint program, const VFMT::VERTEX_DESC* const vertex_desc) {
    for(int i = 0; i < vertex_desc->attribCount; ++i) {
        glBindAttribLocation(program, i, vertex_desc->attribs[i]->in_name);
    }
}
#include "frame_format.hpp"
inline void glxProgramBindFrameFormat(GLuint program, const FFMT::FRAME_DESC* const frame_desc) {
    for(int i = 0; i < frame_desc->layerCount; ++i) {
        glBindFragDataLocation(program, i, frame_desc->layers[i].out_name);    
    }
}
#include "texture_layout.hpp"
inline void glxProgramSetTextureUniforms(GLuint program, const TL::LAYOUT_DESC* const texture_desc) {
    glUseProgram(program);
    for(int i = 0; i < texture_desc->layerCount; ++i) {
        auto& layer = texture_desc->layers[i];
        glUniform1i(glGetUniformLocation(program, layer.shader_name), i);
    }
    glUseProgram(0);
}

class gpuShaderProgram {
    GLuint progid, vid, fid;
public:
    gpuShaderProgram(const char* vs, const char* fs) {
        vid = glCreateShader(GL_VERTEX_SHADER);
        fid = glCreateShader(GL_FRAGMENT_SHADER);
        progid = glCreateProgram();
        glxShaderSource(vid, vs);
        glxShaderSource(fid, fs);
        if(!glxCompileShader(vid)) {}
        if(!glxCompileShader(fid)) {}
        glAttachShader(progid, vid);
        glAttachShader(progid, fid);

        // Attributes
        {
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
            }
        }

        // Outputs
        {
            GLint count = 0;
            int name_len = 0;
            const int NAME_MAX_LEN = 64;
            char name[NAME_MAX_LEN];
            glGetProgramInterfaceiv(progid, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &count);
            for(int i = 0; i < count; ++i) {
                glGetProgramResourceName(progid, GL_PROGRAM_OUTPUT, i, NAME_MAX_LEN, &name_len, name);
                assert(name_len < NAME_MAX_LEN);
                std::string output_name(name, name + name_len);
                glBindFragDataLocation(progid, i, output_name.c_str());
            }
        }
        
        glLinkProgram(progid);
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
        }

        // Uniforms
        {
            GLint count = 0;
            glGetProgramiv(progid, GL_ACTIVE_UNIFORMS, &count);
            int sampler_index = 0;
            for(int i = 0; i < count; ++i) {
                const GLsizei bufSize = 32;
                GLchar name[bufSize];
                GLsizei name_len;
                GLint size;
                GLenum type;
                glGetActiveUniform(progid, (GLuint)i, bufSize, &name_len, &size, &type, name);
                std::string uniform_name(name, name + name_len);
                if(type == GL_SAMPLER_2D_ARB) {
                    GLint loc = glGetUniformLocation(progid, uniform_name.c_str());
                    glUniform1i(loc, sampler_index++);
                }
            }
        }
    }
    ~gpuShaderProgram() {
        glDeleteProgram(progid);
        glDeleteShader(fid);
        glDeleteShader(vid);
    }

    GLuint getId() const {
        return progid;
    }

    GLint getUniformLocation(const char* name) const {
        return glGetUniformLocation(progid, name);
    }
};


#endif
