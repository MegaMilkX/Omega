#ifndef KT_RENDER_MATERIAL_HPP
#define KT_RENDER_MATERIAL_HPP

#include <string>
#include <set>
#include "common/math/gfxm.hpp"
#include "platform/gl/glextutil.h"
#include "glx_shader_program.hpp"
#include "gpu_mesh_desc.hpp"
#include "glx_texture_2d.hpp"
#include "shader_interface.hpp"
#include "gpu_uniform_buffer.hpp"

/*
enum RENDER_TECHNIQUE {
    RENDER_TECHNIQUE_NORMAL,
    RENDER_TECHNIQUE_COUNT
};*/
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

#define GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS 8
class ktRenderPass {
    gpuShaderProgram* prog = 0;
    std::vector<std::unique_ptr<ktRenderPassParam>> params;
public:
    struct {
        uint8_t depth_test : 1;
        uint8_t stencil_test : 1;
        uint8_t cull_faces : 1;
    };

    ktRenderPass()
    : depth_test(1),
    stencil_test(0),
    cull_faces(1)
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
    void setShader(gpuShaderProgram* p) {
        prog = p;
        shaderInterface = SHADER_INTERFACE_GENERIC(p);
        for(int i = 0; i < params.size(); ++i) {
            GLint loc = glGetUniformLocation(prog->getId(), params[i]->getName());
            params[i]->setLocation(loc);
        }
    }

    gpuShaderProgram* getShader() { return prog; }

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
class ktRenderTechnique {
public:
    std::vector<std::unique_ptr<ktRenderPass>> passes;
    int material_local_tech_id;

    ktRenderPass* addPass() {
        passes.push_back(std::unique_ptr<ktRenderPass>(new ktRenderPass()));
        return passes.back().get();
    }

    ktRenderPass* getPass(int i) const {
        return passes[i].get();
    }

    int passCount() const {
        return passes.size();
    }
};


#include "gpu_material_id_pool.hpp"

class gpuPipeline;
class gpuRenderMaterial {
    int guid;
    gpuPipeline* pipeline;
    std::map<std::string, std::unique_ptr<ktRenderTechnique>> techniques_by_name;
    std::vector<int> technique_pipeline_ids;
    std::vector<ktRenderTechnique*> techniques_by_pipeline_id;
    
    std::vector<GLuint> samplers;
    std::map<std::string, int> sampler_names;

    std::vector<gpuUniformBuffer*> uniform_buffers;
public:
    gpuRenderMaterial(gpuPipeline* pipeline)
    : guid(GuidPool<gpuRenderMaterial>::AllocId()), pipeline(pipeline) {
        assert(guid < pow(2, 16));
    }
    ~gpuRenderMaterial() {
        GuidPool<gpuRenderMaterial>::FreeId(guid);
    }

    int getGuid() const {
        return guid;
    }

    ktRenderTechnique* addTechnique(const char* name) {
        assert(techniques_by_name.find(name) == techniques_by_name.end());
        auto ptr = new ktRenderTechnique();
        techniques_by_name[name].reset(ptr);
        return ptr;
    }

    void addSampler(const char* name, gpuTexture2d* texture) {
        sampler_names[name] = samplers.size();
        samplers.push_back(texture->getId());
    }
    void addUniformBuffer(gpuUniformBuffer* buf) {
        uniform_buffers.push_back(buf);
    }

    const ktRenderTechnique* getTechniqueByLocalId(int tech) const {
        auto it = techniques_by_name.begin();
        std::advance(it, tech);
        return it->second.get();
    }
    const ktRenderTechnique* getTechniqueByPipelineId(int tech) const {
        return techniques_by_pipeline_id[tech];
    }

    int techniqueCount() const {
        return techniques_by_name.size();
    }
    int getTechniquePipelineId(int i) const {
        return technique_pipeline_ids[i];
    }

    void compile();

    void bindSamplers() const {
        for (int i = 0; i < samplers.size(); ++i) {
            auto id = samplers[i];
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, id);
        }
    }
    void bindUniformBuffers() const {
        for (int i = 0; i < uniform_buffers.size(); ++i) {
            auto& ub = uniform_buffers[i];
            GLint gl_id = ub->gpu_buf.getId();
            glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
        }
    }
};


#endif
