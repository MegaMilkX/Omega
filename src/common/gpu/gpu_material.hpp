#ifndef KT_RENDER_MATERIAL_HPP
#define KT_RENDER_MATERIAL_HPP

#include <string>
#include <set>
#include "math/gfxm.hpp"
#include "platform/gl/glextutil.h"
#include "gpu_shader_program.hpp"
#include "gpu_mesh_desc.hpp"
#include "gpu_texture_2d.hpp"
#include "gpu/common_resources.hpp"
#include "shader_interface.hpp"
#include "gpu_uniform_buffer.hpp"
#include "util/strid.hpp"

#include "handle/hshared.hpp"

#include <nlohmann/json.hpp>

#include "reflection/reflection.hpp"


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
    HSHARED<gpuShaderProgram> prog;
    
    std::vector<std::unique_ptr<ktRenderPassParam>> params;
    
    
    std::vector<TextureBinding> texture_bindings;
    std::vector<TextureBinding> texture_buffer_bindings;
    std::vector<PassOutputBinding> pass_output_bindings;

public:
    struct {
        uint8_t depth_test : 1;
        uint8_t stencil_test : 1;
        uint8_t cull_faces : 1;
        uint8_t depth_write : 1;
    };
    GPU_BLEND_MODE blend_mode = GPU_BLEND_MODE::NORMAL;

    gpuMaterialPass()
    : depth_test(1),
    stencil_test(0),
    cull_faces(1),
    depth_write(1)
    {}
    SHADER_INTERFACE_GENERIC shaderInterface;
    GLenum gl_draw_buffers[GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS];

    ktRenderPassParam* addParam(const char* name) {
        params.push_back(std::unique_ptr<ktRenderPassParam>(new ktRenderPassParam(name)));
        if(prog) {
            GLint loc = glGetUniformLocation(prog->getId(), params.back()->getName());
            params.back()->setLocation(loc);
        }
        return params.back().get();
    }
    void setShader(HSHARED<gpuShaderProgram> p) {
        prog = p;
        shaderInterface = SHADER_INTERFACE_GENERIC(p.get());
        for(int i = 0; i < params.size(); ++i) {
            GLint loc = glGetUniformLocation(prog->getId(), params[i]->getName());
            params[i]->setLocation(loc);
        }
    }

    gpuShaderProgram* getShader() { return prog.get(); }
    HSHARED<gpuShaderProgram>& getShaderHandle() { return prog; }

    int passOutputBindingCount() const {
        return pass_output_bindings.size();
    }
    const PassOutputBinding& getPassOutputBinding(int idx) const {
        return pass_output_bindings[idx];
    }


    void bindSamplers() {
        for (int i = 0; i < texture_bindings.size(); ++i) {
            auto& binding = texture_bindings[i];
            glActiveTexture(GL_TEXTURE0 + binding.texture_slot);
            glBindTexture(GL_TEXTURE_2D, binding.texture_id);
        }
        for (int i = 0; i < texture_buffer_bindings.size(); ++i) {
            auto& binding = texture_buffer_bindings[i];
            glActiveTexture(GL_TEXTURE0 + binding.texture_slot);
            glBindTexture(GL_TEXTURE_BUFFER, binding.texture_id);
        }
    }
    void bindDrawBuffers() {
        glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, gl_draw_buffers);
    }
    void bindShaderProgram() {
        glUseProgram(prog->getId());
        for (int i = 0; i < params.size(); ++i) {
            auto& p = params[i];
            p->upload();
        }
    }
    void bind() {
        assert(prog);
        bindDrawBuffers();
        bindShaderProgram();
    }

    template<typename UNIFORM>
    void setUniform(typename const UNIFORM::VALUE_T& value) {
        shaderInterface.setUniform<UNIFORM>(value);
    }
};
class gpuMaterialTechnique {
public:
    std::vector<std::unique_ptr<gpuMaterialPass>> passes;
    int material_local_tech_id;

    gpuMaterialPass* addPass() {
        passes.push_back(std::unique_ptr<gpuMaterialPass>(new gpuMaterialPass()));
        return passes.back().get();
    }

    gpuMaterialPass* getPass(int i) const {
        return passes[i].get();
    }

    int passCount() const {
        return passes.size();
    }
};


#include "gpu_material_id_pool.hpp"

class gpuPipeline;
class gpuMaterial {
    int guid;
    std::map<std::string, std::unique_ptr<gpuMaterialTechnique>> techniques_by_name;
    std::vector<int> technique_pipeline_ids;
    std::vector<gpuMaterialTechnique*> techniques_by_pipeline_id;
    
    std::vector<HSHARED<gpuTexture2d>> samplers;
    std::map<std::string, int> sampler_names;
    std::vector<HSHARED<gpuBufferTexture1d>> buffer_samplers;
    std::map<std::string, int> buffer_sampler_names;
    std::set<string_id> pass_output_samplers;

    std::vector<gpuUniformBuffer*> uniform_buffers;

    std::unordered_map<
        gpuMeshBindingKey, 
        std::unique_ptr<gpuMeshMaterialBinding>
    > desc_bindings;
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
        for (auto& kv : techniques_by_name) {
            auto tech = copy->addTechnique(kv.first.c_str());
            for (int i = 0; i < kv.second->passes.size(); ++i) {
                auto pass = tech->addPass();
                pass->blend_mode = kv.second->passes[i]->blend_mode;
                pass->cull_faces = kv.second->passes[i]->cull_faces;
                pass->depth_test = kv.second->passes[i]->depth_test;
                pass->depth_write = kv.second->passes[i]->depth_write;
                pass->stencil_test = kv.second->passes[i]->stencil_test;
                pass->prog = kv.second->passes[i]->prog;
            }
        }
        for (auto& kv : sampler_names) {
            copy->addSampler(kv.first.c_str(), samplers[kv.second]);
        }
        for (auto& kv : buffer_sampler_names) {
            copy->addBufferSampler(kv.first.c_str(), buffer_samplers[kv.second]);
        }
        for (auto& name : pass_output_samplers) {
            copy->addPassOutputSampler(name.to_string().c_str());
        }
        copy->compile();
        return copy;
    }

    gpuMaterialTechnique* addTechnique(const char* name) {
        assert(techniques_by_name.find(name) == techniques_by_name.end());
        auto ptr = new gpuMaterialTechnique();
        techniques_by_name[name].reset(ptr);
        return ptr;
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
    void addPassOutputSampler(const char* name) {
        pass_output_samplers.insert(string_id(name));
    }

    void addUniformBuffer(gpuUniformBuffer* buf) {
        uniform_buffers.push_back(buf);
    }

    const gpuMaterialTechnique* getTechniqueByLocalId(int tech) const {
        auto it = techniques_by_name.begin();
        std::advance(it, tech);
        return it->second.get();
    }
    const std::string& getTechniqueName(int local_id) const {
        auto it = techniques_by_name.begin();
        std::advance(it, local_id);
        if (it == techniques_by_name.end()) {
            static std::string noname = "";
            return noname;
        }
        return it->first;
    }
    const gpuMaterialTechnique* getTechniqueByPipelineId(int tech) const {
        return techniques_by_pipeline_id[tech];
    }

    int techniqueCount() const {
        return techniques_by_name.size();
    }
    int getTechniquePipelineId(int i) const {
        return technique_pipeline_ids[i];
    }

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
