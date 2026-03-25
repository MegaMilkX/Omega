#ifndef KT_RENDER_MATERIAL_HPP
#define KT_RENDER_MATERIAL_HPP

#include <string>
#include <set>
#include <optional>
#include "resource_manager/loadable.hpp"
#include "resource_manager/writable.hpp"
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
class gpuMaterial : public ILoadable, public IWritable {
public:
    struct PARAMETER {
        GLenum type;
        union {
            float float_;
            gfxm::vec2 vec2;
            gfxm::vec3 vec3;
            gfxm::vec4 vec4;
            unsigned char data[36];
        };
        PARAMETER() {}
        PARAMETER(GLenum t) : type(t) {}
    };

private:    
    std::vector<HSHARED<gpuTexture2d>> samplers;
    std::map<std::string, int> sampler_names;
    std::vector<HSHARED<gpuBufferTexture1d>> buffer_samplers;
    std::map<std::string, int> buffer_sampler_names;

    std::vector<gpuUniformBuffer*> uniform_buffers;

    // New stuff
    std::vector<std::unique_ptr<gpuMaterialPass>> passes;
    std::vector<mat_pass_id_t> pipe_pass_to_mat_pass;

    std::map<std::string, PARAMETER> params;

    std::unique_ptr<nlohmann::json> extra_data;

    // New new stuff
    std::optional<GPU_Role> role_override;
    std::optional<bool> transparent;
    std::optional<bool> depth_test;
    std::optional<bool> stencil_test;
    std::optional<bool> cull_faces;
    std::optional<bool> depth_write;
    std::optional<GPU_BLEND_MODE> blend_mode;
    ResourceRef<gpuShaderSet> vertex_extension_set;
    ResourceRef<gpuShaderSet> fragment_extension_set;

public:
    TYPE_ENABLE();

    gpuMaterial() {}
    ~gpuMaterial() {}

    void setRoleOverride(GPU_Role role) { role_override = role; }
    std::optional<GPU_Role> getRoleOverride() const { return role_override; }

    void setTransparent(bool t) { transparent = t; }
    std::optional<bool> getTransparent() const { return transparent; }

    void setBlendingMode(GPU_BLEND_MODE m) { blend_mode = m; }
    std::optional<GPU_BLEND_MODE> getBlendingMode() const { return blend_mode; }

    void setDepthTest(bool v) { depth_test = v; }
    void setDepthWrite(bool v) { depth_write = v; }
    void setStencilTest(bool v) { stencil_test = v; }
    void setBackfaceCulling(bool v) { cull_faces = v; }
    std::optional<bool> getDepthTest() const { return depth_test; }
    std::optional<bool> getDepthWrite() const { return depth_write; }
    std::optional<bool> getStencilTest() const { return stencil_test; }
    std::optional<bool> getBackfaceCulling() const { return cull_faces; }

    void setVertexExtension(const ResourceRef<gpuShaderSet>& set) {
        vertex_extension_set = set;
    }
    void setFragmentExtension(const ResourceRef<gpuShaderSet>& set) {
        fragment_extension_set = set;
    }
    bool hasVertexExtensionSet() const { return vertex_extension_set; }
    bool hasFragmentExtensionSet() const { return fragment_extension_set; }
    gpuShaderSet* getVertexExtensionSet() const { return const_cast<gpuShaderSet*>(vertex_extension_set.get()); }
    gpuShaderSet* getFragmentExtensionSet() const { return const_cast<gpuShaderSet*>(fragment_extension_set.get()); }
    ResourceRef<gpuShaderSet> getVertexExtensionRef() const { return vertex_extension_set; }
    ResourceRef<gpuShaderSet> getFragmentExtensionRef() const { return fragment_extension_set; }

    nlohmann::json* getExtraData() {
        if (!extra_data) {
            extra_data.reset(new nlohmann::json);
        }
        return extra_data.get();
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
    int getSamplerIdx(const char* name) const {
        auto it = sampler_names.find(name);
        if (it == sampler_names.end()) {
            return -1;
        }
        return it->second;
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
    const std::map<std::string, PARAMETER>& getParams() const { return params; }

    void compile();

    void bindUniformBuffers() const {
        for (int i = 0; i < uniform_buffers.size(); ++i) {
            auto& ub = uniform_buffers[i];
            GLint gl_id = ub->gpu_buf.getId();
            glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
        }
    }

    DEFINE_EXTENSIONS(e_mat, e_material);
    bool load(byte_reader& in) override;
    void write(byte_writer& out) const override;
};


#endif
