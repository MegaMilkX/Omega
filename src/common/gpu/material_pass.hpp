#pragma once

#include <vector>
#include <memory>
#include "math/gfxm.hpp"
#include "platform/gl/glextutil.h"
#include "util/strid.hpp"
#include "gpu_shader_program.hpp"
#include "gpu/common/shader_sampler_set.hpp"
#include "shader_interface.hpp"

class ktRenderPassParam {
public:
    enum TYPE {
        NONE, INT, FLOAT, VEC2, VEC3, VEC4, MAT3, MAT4
    };
private:
    std::string name;
    GLint location = -1;
    TYPE type;
    union {
        int _int;
        float _float;
        gfxm::vec2 _vec2;
        gfxm::vec3 _vec3;
        gfxm::vec4 _vec4;
        gfxm::mat3 _mat3;
        gfxm::mat4 _mat4;  
    };
public:
    ktRenderPassParam(const char* name)
    : name(name) {}

    const char* getName() const { return name.c_str(); }
    int getLocation() const { return location; }

    void setLocation(int loc) { location = loc; }

    TYPE getType() const { return type; }

    void upload() const {
        switch(type) {
        case INT:
            glUniform1i(location, _int);
            break;
        case FLOAT:
            glUniform1f(location, _float);
            break;
        case VEC2:
            glUniform2fv(location, 1, (float*)&_vec2);
            break;
        case VEC3:
            glUniform3fv(location, 1, (float*)&_vec3);
            break;
        case VEC4:
            glUniform4fv(location, 1, (float*)&_vec4);
            break;
        case MAT3:
            glUniformMatrix3fv(location, 1, GL_FALSE, (float*)&_mat3);
            break;
        case MAT4:
            glUniformMatrix4fv(location, 1, GL_FALSE, (float*)&_mat4);
            break;
        }
    }

    bool isInt() const { return type == INT; }
    bool isFloat() const { return type == FLOAT; }
    bool isVec2() const { return type == VEC2; }
    bool isVec3() const { return type == VEC3; }
    bool isVec4() const { return type == VEC4; }
    bool isMat3() const { return type == MAT3; }
    bool isMat4() const { return type == MAT4; }
    void setInt(int value) { _int = value; type = INT; }
    void setFloat(float value) { _float = value; type = FLOAT; }
    void setVec2(const gfxm::vec2& value) { _vec2 = value; type = VEC2; }
    void setVec3(const gfxm::vec3& value) { _vec3 = value; type = VEC3; }
    void setVec4(const gfxm::vec4& value) { _vec4 = value; type = VEC4; }
    void setMat3(const gfxm::mat3& value) { _mat3 = value; type = MAT3; }
    void setMat4(const gfxm::mat4& value) { _mat4 = value; type = MAT4; }
    int         getInt() const { return _int; }
    float       getFloat() const { return _float; }
    gfxm::vec2  getVec2() const { return _vec2; }
    gfxm::vec3  getVec3() const { return _vec3; }
    gfxm::vec4  getVec4() const { return _vec4; }
    gfxm::mat3  getMat3() const { return _mat3; }
    gfxm::mat4  getMat4() const { return _mat4; }
};


enum class GPU_BLEND_MODE {
    NORMAL,
    ADD,
    MULTIPLY
};

#define GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS 8
class gpuMaterial;
class gpuMaterialPass {
    friend gpuMaterial;
public:
    struct TextureBinding {
        GLuint texture_id;
        GLint  texture_slot;
    };
    struct PassOutputBinding {
        string_id   strid;
        GLint       texture_slot;
    };
private:
    int pipeline_idx = 0;
    std::string path;
    HSHARED<gpuShaderProgram> prog;
    
    std::vector<std::unique_ptr<ktRenderPassParam>> params;    
    
    // Compiled
    ShaderSamplerSet sampler_set;

public:
    struct {
        uint8_t depth_test : 1;
        uint8_t stencil_test : 1;
        uint8_t cull_faces : 1;
        uint8_t depth_write : 1;
    };
    GPU_BLEND_MODE blend_mode = GPU_BLEND_MODE::NORMAL;

    gpuMaterialPass(const char* path);

    SHADER_INTERFACE_GENERIC shaderInterface;
    GLenum gl_draw_buffers[GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS];

    int getPipelineIdx() const;
    const std::string& getPath() const;

    const ShaderSamplerSet& getSamplerSet() const;

    ktRenderPassParam* addParam(const char* name);

    void setShaderProgram(HSHARED<gpuShaderProgram> p);

    gpuShaderProgram* getShaderProgram();
    HSHARED<gpuShaderProgram>& getShaderProgramHandle();

    void bindDrawBuffers();
    void bindShaderProgram();
    void bind();

    template<typename UNIFORM>
    void setUniform(typename const UNIFORM::VALUE_T& value) {
        shaderInterface.setUniform<UNIFORM>(value);
    }
};

