#include "gpu_renderable.hpp"

#include "gpu.hpp"
#include "gpu_pipeline.hpp"


void gpuRenderable::enableMaterialTechnique(const char* path, bool value) {
    auto node = gpuGetPipeline()->findNode(path);
    if (!node) {
        assert(false);
        return;
    }
    int pass_count = node->getPassCount();
    gpuPass* passes[32];
    pass_count = node->getPassList(passes, 32);
    for (int i = 0; i < pass_count; ++i) {
        int pass_pipe_idx = passes[i]->getId();
        int pass_mat_idx = material->getPassMaterialIdx(pass_pipe_idx);
        if (pass_mat_idx < 0) {
            continue;
        }
        pass_states[pass_mat_idx] = value;
    }
}

int gpuRenderable::getParameterIndex(const char* name) {
    auto it = param_indices.find(name);
    if (it == param_indices.end()) {
        return -1;
    }
    return it->second;
}

void gpuRenderable::setParam(int index, GLenum type, void* pvalue) {
    auto& p = params[index];
    if (p.type == PARAM_UNIFORM) {
        for (int i = 0; i < p.uniform_data_indices.size(); ++i) {
            auto& u = uniform_data[p.uniform_data_indices[i]];
            u.prog->getUniformInfo(u.program_index).auto_upload = true;
            memcpy(u.data, pvalue, glTypeToSize(type));
        }
    }
}
void gpuRenderable::setFloat(int index, float value) {
    auto& p = params[index];
    if (p.type == PARAM_UNIFORM) {
        for (int i = 0; i < p.uniform_data_indices.size(); ++i) {
            auto& u = uniform_data[p.uniform_data_indices[i]];
            u.prog->getUniformInfo(u.program_index).auto_upload = true;
            u.float_ = value;
        }
    }
}
void gpuRenderable::setVec2(int index, const gfxm::vec2& value) {
    auto& p = params[index];
    if (p.type == PARAM_UNIFORM) {
        for (int i = 0; i < p.uniform_data_indices.size(); ++i) {
            auto& u = uniform_data[p.uniform_data_indices[i]];
            u.prog->getUniformInfo(u.program_index).auto_upload = true;
            u.vec2 = value;
        }
    }
}
void gpuRenderable::setVec3(int index, const gfxm::vec3& value) {
    auto& p = params[index];
    if (p.type == PARAM_UNIFORM) {
        for (int i = 0; i < p.uniform_data_indices.size(); ++i) {
            auto& u = uniform_data[p.uniform_data_indices[i]];
            u.prog->getUniformInfo(u.program_index).auto_upload = true;
            u.vec3 = value;
        }
    }

}
void gpuRenderable::setVec4(int index, const gfxm::vec4& value) {
    auto& p = params[index];
    if (p.type == PARAM_UNIFORM) {
        for (int i = 0; i < p.uniform_data_indices.size(); ++i) {
            auto& u = uniform_data[p.uniform_data_indices[i]];
            u.prog->getUniformInfo(u.program_index).auto_upload = true;
            u.vec4 = value;
        }
    }
}
void gpuRenderable::setQuat(int index, const gfxm::quat& value) {
    auto& p = params[index];
    if (p.type == PARAM_UNIFORM) {
        for (int i = 0; i < p.uniform_data_indices.size(); ++i) {
            auto& u = uniform_data[p.uniform_data_indices[i]];
            u.prog->getUniformInfo(u.program_index).auto_upload = true;
            u.vec4 = gfxm::vec4(value.x, value.y, value.z, value.w);
        }
    }
}
void gpuRenderable::setMat3(int index, const gfxm::mat3& value) {
    auto& p = params[index];
    if (p.type == PARAM_UNIFORM) {
        for (int i = 0; i < p.uniform_data_indices.size(); ++i) {
            auto& u = uniform_data[p.uniform_data_indices[i]];
            u.prog->getUniformInfo(u.program_index).auto_upload = true;
            u.mat3 = value;
        }
    }
}
void gpuRenderable::setMat4(int index, const gfxm::mat4& value) {
    auto& p = params[index];
    if (p.type == PARAM_UNIFORM) {
        for (int i = 0; i < p.uniform_data_indices.size(); ++i) {
            auto& u = uniform_data[p.uniform_data_indices[i]];
            u.prog->getUniformInfo(u.program_index).auto_upload = true;
            u.mat4 = value;
        }
    }
}

void gpuRenderable::setFloat(const char* name, float value) {
    setFloat(getParameterIndex(name), value);
}
void gpuRenderable::setVec2(const char* name, const gfxm::vec2& value) {
    setVec2(getParameterIndex(name), value);
}
void gpuRenderable::setVec3(const char* name, const gfxm::vec3& value) {
    setVec3(getParameterIndex(name), value);
}
void gpuRenderable::setVec4(const char* name, const gfxm::vec4& value) {
    setVec4(getParameterIndex(name), value);
}
void gpuRenderable::setQuat(const char* name, const gfxm::quat& value) {
    setQuat(getParameterIndex(name), value);
}
void gpuRenderable::setMat3(const char* name, const gfxm::mat3& value) {
    setMat3(getParameterIndex(name), value);
}
void gpuRenderable::setMat4(const char* name, const gfxm::mat4& value) {
    setMat4(getParameterIndex(name), value);
}

gpuGeometryRenderable::gpuGeometryRenderable(gpuMaterial* mat, const gpuMeshDesc* mesh, const gpuInstancingDesc* instancing, const char* dbg_name)
    : gpuRenderable(mat, mesh, instancing, dbg_name) {
    ubuf_model = gpuGetPipeline()->createUniformBuffer(UNIFORM_BUFFER_MODEL);
    loc_transform = ubuf_model->getDesc()->getUniform(UNIFORM_MODEL_TRANSFORM);
    attachUniformBuffer(ubuf_model);
    compile();
}
gpuGeometryRenderable::~gpuGeometryRenderable() {
    gpuGetPipeline()->destroyUniformBuffer(ubuf_model);
}