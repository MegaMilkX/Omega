#pragma once

#include <vector>
#include <memory>
#include "math/gfxm.hpp"
#include "platform/gl/glextutil.h"
#include "util/strid.hpp"
#include "gpu_shader_program.hpp"
#include "gpu/common/shader_sampler_set.hpp"
#include "shader_interface.hpp"


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

