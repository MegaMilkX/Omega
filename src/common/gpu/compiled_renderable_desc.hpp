#pragma once

#include "handle/hshared.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/common/shader_sampler_set.hpp"


struct gpuAttribBinding {
    const gpuBuffer* buffer;
    int location;
    int count;
    int stride;
    int offset;
    GLenum gl_type;
    bool normalized;
    bool is_instance_array;
    // Debug
    VFMT::GUID guid;

    gpuAttribBinding& operator=(const gpuAttribBinding& other) {
        buffer = other.buffer;
        location = other.location;
        count = other.count;
        stride = other.stride;
        offset = other.offset;
        gl_type = other.gl_type;
        normalized = other.normalized;
        is_instance_array = other.is_instance_array;
        guid = other.guid;
        return *this;
    }
};
struct gpuMeshShaderBinding {
    GLuint vao = 0;
    std::vector<gpuAttribBinding> attribs;
    const gpuBuffer* index_buffer;
    int vertex_count = 0;
    int index_count = 0;
    MESH_DRAW_MODE draw_mode;

    // TODO: gpuMeshShaderBinding are copied somewhere and used without the material binding
    /*
    ~gpuMeshShaderBinding() {
        if (vao) {
            glDeleteVertexArrays(1, &vao);
        }
    }*/
};

struct gpuCompiledRenderableDesc {
    struct PassDesc {
        RHSHARED<gpuShaderProgram> prog;
        pipe_pass_id_t pass;
        mat_pass_id_t material_pass;
        gpuMeshShaderBinding binding;
        GLenum gl_draw_buffers[GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS];
        ShaderSamplerSet sampler_set;
        draw_flags_t draw_flags = 0;
        GPU_BLEND_MODE blend_mode;

        uint32_t sampler_set_identity;
        uint32_t state_identity;
    };

    std::vector<PassDesc> pass_array;

    ~gpuCompiledRenderableDesc() {
        // TODO: gpuMeshShaderBinding are copied somewhere and used without the material binding
        for (int i = 0; i < pass_array.size(); ++i) {
            if (pass_array[i].binding.vao) {
                glDeleteVertexArrays(1, &pass_array[i].binding.vao);
            }
        }
    }
};


// NOTE: Basically glBindVertexArray() if there was an actual VAO
// keeping it this way for now
void gpuBindMeshBinding(const gpuMeshShaderBinding* b);
void gpuBindMeshBindingDirect(const gpuMeshShaderBinding* b);
void gpuDrawMeshBinding(const gpuMeshShaderBinding* b);
void gpuDrawMeshBindingInstanced(const gpuMeshShaderBinding* binding, int instance_count);

class gpuRenderable;
class gpuMaterial;
class gpuShaderProgram;
class gpuMeshDesc;
class gpuInstancingDesc;
gpuMeshShaderBinding* gpuCreateMeshShaderBinding(
    const gpuShaderProgram* prog,
    const gpuMeshDesc* desc,
    const gpuInstancingDesc* inst_desc = 0
);
void gpuDestroyMeshShaderBinding(
    gpuMeshShaderBinding* binding
);
bool gpuMakeMeshShaderBinding(
    gpuMeshShaderBinding* out_binding,
    const gpuShaderProgram* prog,
    const gpuMeshDesc* desc,
    const gpuInstancingDesc* inst_desc = 0
);

bool gpuCompileRenderablePasses(
    gpuCompiledRenderableDesc* binding,
    const gpuRenderable* renderable,
    const gpuMaterial* material,
    const gpuMeshDesc* desc,
    const gpuInstancingDesc* inst_desc = 0
);

