#ifndef KT_RENDER_MATERIAL_HPP
#define KT_RENDER_MATERIAL_HPP

#include "gpu_material.auto.hpp"

#include <string>
#include <set>
#include <optional>
#include "resource_manager/resource.hpp"
#include "resource_manager/loadable.hpp"
#include "resource_manager/writable.hpp"
#include "math/gfxm.hpp"
#include "platform/gl/glextutil.h"
#include "gpu/gpu_types.hpp"
#include "gpu/types.hpp"
#include "gpu_shader_program.hpp"
#include "gpu_mesh_desc.hpp"
#include "texture/texture2d.hpp"
#include "texture/buffer_texture.hpp"
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

#include "resource_manager/resource_root.hpp"

#include "gpu/material_resource_backend.hpp"


int glTypeToSize(GLenum type);

enum class GPU_ShadingStyle {
    Opaque,
    ForwardTranslucent,
    VFX,
    WATER, // ?

    COUNT
};


enum class GPU_MaterialPassReqType {
    None,
    Explicit,
    Style
};
struct GPU_MaterialPassReq {
    GPU_MaterialPassReqType type;
    const char* pass_name = nullptr;
    GPU_ShadingStyle style;

    GPU_MaterialPassReq()
        :type(GPU_MaterialPassReqType::None), pass_name(nullptr), style(GPU_ShadingStyle::Opaque) {}
    GPU_MaterialPassReq(const char* pass)
        : type(GPU_MaterialPassReqType::Explicit), pass_name(pass), style(GPU_ShadingStyle::Opaque) {}
    GPU_MaterialPassReq(GPU_ShadingStyle style)
        : type(GPU_MaterialPassReqType::Style), pass_name(nullptr), style(style) {}
};


[[cppi_enum]];
enum class GPU_UVScrollMode {
    None,
    Smooth,
    Step,
    Flipbook
};


class gpuMaterial;
RESOURCE_BACKEND(gpuMaterial, MaterialResourceBackend);

class gpuPipeline;
[[cppi_class]];
class gpuMaterial :
    public Resource,
    public rtti::MetaObject,
    public ILoadable,
    public IWritable,
    public PolymorphicResourceRoot<gpuMaterial>
{
protected:
    GPU_ShadingStyle shading_style = GPU_ShadingStyle::Opaque;
    GPU_MaterialPassReq pass_requirement;
    bool is_animated = false;

    void registerVertexSet(const ResourceRef<gpuShaderSet>& shaders);
    void registerFragmentSet(const ResourceRef<gpuShaderSet>& shaders);
    void registerShaderKey(const ShaderKey* key) { p_shader_key = key; }

    void touchVersion() { ++version; }
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
    int version = 0;
    std::vector<ResourceRef<gpuTexture2d>> samplers;
    std::map<std::string, int> sampler_names;
    std::vector<HSHARED<gpuBufferTexture>> buffer_samplers;
    std::map<std::string, int> buffer_sampler_names;

    std::vector<gpuUniformBuffer*> uniform_buffers;

    // New stuff
    std::vector<std::unique_ptr<gpuMaterialPass>> passes;

    std::map<std::string, PARAMETER> params;

    std::unique_ptr<nlohmann::json> extra_data;

    // New new stuff
    const ShaderKey* p_shader_key = nullptr;
    std::optional<GPU_Role> role_override = std::nullopt;
    bool transparent = false;
    bool depth_test = true;
    bool stencil_test = false;
    bool cull_faces = true;
    bool depth_write = true;
    GPU_BLEND_MODE blend_mode = GPU_BLEND_MODE::BLEND;
    int sort_bias = 0;

    ResourceRef<gpuShaderSet> vertex_set = nullptr;
    ResourceRef<gpuShaderSet> fragment_set = nullptr;
public:
    TYPE_ENABLE();

    int tick_frame_id = 0;

    gpuMaterial();
    ~gpuMaterial() {}

    virtual bool resolvePass(GPU_RenderDomain domain, PassResolution& out) const { return false; }
    virtual void applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) {}
    virtual void onTick(float dt) {}

    int getVersion() const { return version; }

    const ShaderKey* getShaderKey() const { return p_shader_key; }

    GPU_ShadingStyle getShadingStyle() const { return shading_style; }
    GPU_MaterialPassReq getPassRequirement() const { return pass_requirement; }

    bool isAnimated() const { return is_animated; }

    void setRoleOverride(GPU_Role role) { role_override = role; }
    std::optional<GPU_Role> getRoleOverride() const { return role_override; }

    void setTransparent(bool t) { transparent = t; }
    bool getTransparent() const { return transparent; }

    void setBlendingMode(GPU_BLEND_MODE m) { blend_mode = m; }
    GPU_BLEND_MODE getBlendingMode() const { return blend_mode; }

    void setDepthTest(bool v) { depth_test = v; }
    void setDepthWrite(bool v) { depth_write = v; }
    void setStencilTest(bool v) { stencil_test = v; }
    void setBackfaceCulling(bool v) { cull_faces = v; }
    void setSortBias(int b) { sort_bias = b; }
    bool getDepthTest() const { return depth_test; }
    bool getDepthWrite() const { return depth_write; }
    bool getStencilTest() const { return stencil_test; }
    bool getBackfaceCulling() const { return cull_faces; }
    int getSortBias() const { return sort_bias; }

    bool hasVertexShaders() const { return vertex_set; }
    bool hasFragmentShaders() const { return fragment_set; }
    gpuShaderSet* getVertexShaders() const { return const_cast<gpuShaderSet*>(vertex_set.get()); }
    gpuShaderSet* getFragmentShaders() const { return const_cast<gpuShaderSet*>(fragment_set.get()); }

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

    void addSampler(const char* name, ResourceRef<gpuTexture2d> texture) {
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
    ResourceRef<gpuTexture2d> getSampler(const char* name) const {
        auto it = sampler_names.find(name);
        if (it == sampler_names.end()) {
            return getDefaultTexture(name);
        }
        return samplers[it->second];
    }
    ResourceRef<gpuTexture2d>& getSampler(int i) {
        auto it = sampler_names.begin();
        std::advance(it, i);
        if (it == sampler_names.end()) {
            static ResourceRef<gpuTexture2d> tmp;
            return tmp;
        }
        return samplers[it->second];
    }
    const ResourceRef<gpuTexture2d>& getSampler(int i) const {
        auto it = sampler_names.begin();
        std::advance(it, i);
        if (it == sampler_names.end()) {
            static ResourceRef<gpuTexture2d> tmp;
            return tmp;
        }
        return samplers[it->second];
    }
    const std::string& getSamplerName(int i) const {
        auto it = sampler_names.begin();
        std::advance(it, i);
        if (it == sampler_names.end()) {
            static std::string noname = "";
            return noname;
        }
        return it->first;
    }

    void addBufferSampler(const char* name, HSHARED<gpuBufferTexture> texture) {
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
    HSHARED<gpuBufferTexture>& getBufferSampler(int i) {
        auto it = buffer_sampler_names.begin();
        std::advance(it, i);
        if (it == buffer_sampler_names.end()) {
            static HSHARED<gpuBufferTexture> tmp(0);
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

    void bindUniformBuffers();

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
            glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->binding_location, gl_id);
        }
    }

    void makeSnapshot(rtti::PropSnapshot&) const override;
    void applySnapshot(const rtti::PropSnapshot&) override;

    virtual void toJson(nlohmann::json&) const;
    virtual bool fromJson(const nlohmann::json&);

    DEFINE_EXTENSIONS(e_mat, e_material);
    bool load(byte_reader& in) override;
    void write(const std::string& path) const;
    void write(byte_writer& out) const override;
};


#endif
