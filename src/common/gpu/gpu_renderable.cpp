#include "gpu_renderable.hpp"

#include "gpu.hpp"
#include "gpu_pipeline.hpp"


GLenum gpuTypeToGLenum(GPU_TYPE type) {
    switch (type) {
    case GPU_FLOAT: return GL_FLOAT;
    case GPU_VEC2: return GL_FLOAT_VEC2;
    case GPU_VEC3: return GL_FLOAT_VEC3;
    case GPU_VEC4: return GL_FLOAT_VEC4;
    case GPU_MAT3: return GL_FLOAT_MAT3;
    case GPU_MAT4: return GL_FLOAT_MAT4;
    default:
        assert(false);
        return 0;
    }
}


gpuRenderable::~gpuRenderable() {
    for (int i = 0; i < owned_buffers.size(); ++i) {
        gpuGetPipeline()->destroyUniformBuffer(owned_buffers[i]);
    }
}

gpuUniformBuffer* gpuRenderable::getOrCreateUniformBuffer(const char* name) {
    for (int i = 0; i < uniform_buffers.size(); ++i) {
        if (uniform_buffers[i]->getDesc()->getName() == std::string(name)) {
            return uniform_buffers[i];
        }
    }

    if (gpuGetPipeline()->isUniformBufferAttached(name)) {
        return 0;
    }
    
    gpuUniformBuffer* buf = gpuGetPipeline()->createUniformBuffer(name);
    owned_buffers.push_back(buf);
    attachUniformBuffer(buf);
    return buf;
}

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

void gpuRenderable::setParam(int index, GPU_TYPE type, const void* pvalue) {
    setParam(index, gpuTypeToGLenum(type), pvalue);
}
void gpuRenderable::setParam(int index, GLenum type, const void* pvalue) {
    auto& p = params[index];
    if (p.type == PARAM_UNIFORM) {
        for (int i = 0; i < p.uniform_data_indices.size(); ++i) {
            auto& u = uniform_data[p.uniform_data_indices[i]];
            u.prog->getUniformInfo(u.program_index).auto_upload = true;
            memcpy(u.data, pvalue, glTypeToSize(type));
        }
    } else if (p.type == PARAM_BLOCK_FIELD) {
        if(p.buffer) {
            p.buffer->setValue(p.ub_offset, pvalue, glTypeToSize(type));
        }
    } else {
        assert(false);
    }
}
void gpuRenderable::setFloat(int index, float value) {
    setParam(index, GL_FLOAT, &value);
}
void gpuRenderable::setVec2(int index, const gfxm::vec2& value) {
    setParam(index, GL_FLOAT_VEC2, &value);
}
void gpuRenderable::setVec3(int index, const gfxm::vec3& value) {
    setParam(index, GL_FLOAT_VEC3, &value);
}
void gpuRenderable::setVec4(int index, const gfxm::vec4& value) {
    setParam(index, GL_FLOAT_VEC4, &value);
}
void gpuRenderable::setQuat(int index, const gfxm::quat& value) {
    setParam(index, GL_FLOAT_VEC4, &value);
}
void gpuRenderable::setMat3(int index, const gfxm::mat3& value) {
    setParam(index, GL_FLOAT_MAT3, &value);
}
void gpuRenderable::setMat4(int index, const gfxm::mat4& value) {
    setParam(index, GL_FLOAT_MAT4, &value);
}

void gpuRenderable::setParam(const char* name, GPU_TYPE type, const void* pvalue) {
    setParam(getParameterIndex(name), type, pvalue);
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


gpuRenderable& gpuRenderable::attachUniformBuffer(gpuUniformBuffer* buf) {
    for (int i = 0; i < uniform_buffers.size(); ++i) {
        if (uniform_buffers[i]->getDesc() == buf->getDesc()) {
            for (int j = 0; j < owned_buffers.size(); ++j) {
                gpuGetPipeline()->destroyUniformBuffer(owned_buffers[j]);
                owned_buffers.erase(owned_buffers.begin() + j);
                break;
            }
            uniform_buffers[i] = buf;
            return *this;
        }
    }
    uniform_buffers.push_back(buf);
    return *this;
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