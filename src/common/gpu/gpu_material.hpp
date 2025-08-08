#ifndef KT_RENDER_MATERIAL_HPP
#define KT_RENDER_MATERIAL_HPP

#include <string>
#include <set>
#include "math/gfxm.hpp"
#include "platform/gl/glextutil.h"
#include "gpu/gpu_types.hpp"
#include "gpu_shader_program.hpp"
#include "gpu_mesh_desc.hpp"
#include "gpu_texture_2d.hpp"
#include "gpu/common_resources.hpp"
#include "shader_interface.hpp"
#include "gpu_uniform_buffer.hpp"
#include "gpu/common/shader_sampler_set.hpp"
#include "util/strid.hpp"

#include "handle/hshared.hpp"

#include <nlohmann/json.hpp>

#include "reflection/reflection.hpp"

#include "gpu_material_id_pool.hpp"

#include "material_pass.hpp"


int glTypeToSize(GLenum type);

class gpuPipeline;
class gpuMaterial {
public:
    struct PARAMETER {
        GLenum type;
        union {
            unsigned char data[36];
        };
    };

private:
    int guid;
    
    std::vector<HSHARED<gpuTexture2d>> samplers;
    std::map<std::string, int> sampler_names;
    std::vector<HSHARED<gpuBufferTexture1d>> buffer_samplers;
    std::map<std::string, int> buffer_sampler_names;

    std::vector<gpuUniformBuffer*> uniform_buffers;

    std::unordered_map<
        gpuMeshBindingKey, 
        std::unique_ptr<gpuMeshMaterialBinding>
    > desc_bindings;

    // New stuff
    std::vector<std::unique_ptr<gpuMaterialPass>> passes;
    std::vector<mat_pass_id_t> pipe_pass_to_mat_pass;

    std::map<std::string, PARAMETER> params;

public:
    TYPE_ENABLE();

    gpuMaterial()
    : guid(GuidPool<gpuMaterial>::AllocId()) {
        assert(guid < pow(2, 16));
    }
    ~gpuMaterial() {
        GuidPool<gpuMaterial>::FreeId(guid);
    }

    int getGuid() const {
        return guid;
    }

    RHSHARED<gpuMaterial> makeCopy() {
        RHSHARED<gpuMaterial> copy;
        copy.reset_acquire();
        
        for (int i = 0; i < passes.size(); ++i) {
            auto pass_from = passes[i].get();
            auto pass_to = copy->addPass(pass_from->getPath().c_str());
            pass_to->blend_mode = pass_from->blend_mode;
            pass_to->cull_faces = pass_from->cull_faces;
            pass_to->depth_test = pass_from->depth_test;
            pass_to->depth_write = pass_from->depth_write;
            pass_to->stencil_test = pass_from->stencil_test;
            pass_to->prog = pass_from->prog;
        }

        for (auto& kv : sampler_names) {
            copy->addSampler(kv.first.c_str(), samplers[kv.second]);
        }
        for (auto& kv : buffer_sampler_names) {
            copy->addBufferSampler(kv.first.c_str(), buffer_samplers[kv.second]);
        }
        copy->compile();
        return copy;
    }

    gpuMaterialPass* addPass(const char* path) {
        passes.push_back(std::unique_ptr<gpuMaterialPass>(new gpuMaterialPass(path)));
        return passes.back().get();
    }

    gpuMaterialPass* getPass(mat_pass_id_t i) const {
        return passes[i].get();
    }

    int passCount() const {
        return passes.size();
    }

    mat_pass_id_t getPassMaterialIdx(pipe_pass_id_t i) const {
        return pipe_pass_to_mat_pass[i];
    }

    void addSampler(const char* name, HSHARED<gpuTexture2d> texture) {
        auto it = sampler_names.find(name);
        if (it != sampler_names.end()) {
            samplers[it->second] = texture;
        } else {
            sampler_names[name] = samplers.size();
            samplers.push_back(texture);
        }
    }
    size_t samplerCount() const {
        return sampler_names.size();
    }
    HSHARED<gpuTexture2d> getSampler(const char* name) const {
        auto it = sampler_names.find(name);
        if (it == sampler_names.end()) {
            return getDefaultTexture(name);
        }
        return samplers[it->second];
    }
    HSHARED<gpuTexture2d>& getSampler(int i) {
        auto it = sampler_names.begin();
        std::advance(it, i);
        if (it == sampler_names.end()) {
            static HSHARED<gpuTexture2d> tmp;
            return tmp;
        }
        return samplers[it->second];
    }
    const std::string& getSamplerName(int i) {
        auto it = sampler_names.begin();
        std::advance(it, i);
        if (it == sampler_names.end()) {
            static std::string noname = "";
            return noname;
        }
        return it->first;
    }

    void addBufferSampler(const char* name, HSHARED<gpuBufferTexture1d> texture) {
        auto it = buffer_sampler_names.find(name);
        if (it != buffer_sampler_names.end()) {
            buffer_samplers[it->second] = texture;
        }
        else {
            buffer_sampler_names[name] = buffer_samplers.size();
            buffer_samplers.push_back(texture);
        }
    }
    size_t bufferSamplerCount() const {
        return buffer_sampler_names.size();
    }
    HSHARED<gpuBufferTexture1d>& getBufferSampler(int i) {
        auto it = buffer_sampler_names.begin();
        std::advance(it, i);
        if (it == buffer_sampler_names.end()) {
            static HSHARED<gpuBufferTexture1d> tmp(0);
            return tmp;
        }
        return buffer_samplers[it->second];
    }
    const std::string& getBufferSamplerName(int i) {
        auto it = buffer_sampler_names.begin();
        std::advance(it, i);
        if (it == buffer_sampler_names.end()) {
            static std::string noname = "";
            return noname;
        }
        return it->first;
    }

    void addUniformBuffer(gpuUniformBuffer* buf) {
        uniform_buffers.push_back(buf);
    }

    void setParam(const std::string& name, GLenum type, const void* data);
    void setParamFloat(const std::string& name, float value);
    void setParamVec2(const std::string& name, const gfxm::vec2& v);
    void setParamVec3(const std::string& name, const gfxm::vec3& v);
    void setParamVec4(const std::string& name, const gfxm::vec4& v);
    void setParamInt(const std::string& name, int value);
    void setParamVec2i(const std::string& name, const gfxm::ivec2& v);
    void setParamVec3i(const std::string& name, const gfxm::ivec3& v);
    void setParamVec4i(const std::string& name, const gfxm::ivec4& v);
    void setParamMat2(const std::string& name, float* pvalue);
    void setParamMat3(const std::string& name, float* pvalue);
    void setParamMat4(const std::string& name, float* pvalue);
    void setParamMat2x3(const std::string& name, float* pvalue);
    void setParamMat2x4(const std::string& name, float* pvalue);
    void setParamMat3x2(const std::string& name, float* pvalue);
    void setParamMat3x4(const std::string& name, float* pvalue);
    void setParamMat4x2(const std::string& name, float* pvalue);
    void setParamMat4x3(const std::string& name, float* pvalue);

    PARAMETER* getParam(const std::string& name);

    void compile();

    void bindUniformBuffers() const {
        for (int i = 0; i < uniform_buffers.size(); ++i) {
            auto& ub = uniform_buffers[i];
            GLint gl_id = ub->gpu_buf.getId();
            glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
        }
    }

    void serializeJson(nlohmann::json& j);
    void deserializeJson(nlohmann::json& j);
};


#endif
