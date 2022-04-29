#pragma once

#include <vector>
#include <memory>
#include <map>
#include "platform/gl/glextutil.h"
#include "common/render/glx_texture_2d.hpp"
#include "common/render/gpu_uniform_buffer.hpp"
#include "common/render/gpu_material.hpp"

class gpuRenderBuffer {
public:
    gpuRenderBuffer(/*TODO*/) {

    }
};

class gpuFrameBuffer {
    unsigned int fbo;

    struct ColorTarget {
        std::string name;
        int index;
    };
    std::vector<ColorTarget> color_targets;
public:
    gpuFrameBuffer() {
        glGenFramebuffers(1, &fbo);
    }
    ~gpuFrameBuffer() {
        glDeleteFramebuffers(1, &fbo);
    }
    void addColorTarget(const char* name, gpuTexture2d* texture) {
        int index = color_targets.size();

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, texture->getId(), 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        color_targets.push_back(ColorTarget{ std::string(name), index });
    }
    void addDepthTarget(gpuTexture2d* texture) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->getId(), 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void prepare() {
        // TODO: !!!
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        GLenum draw_buffers[] = {
            GL_COLOR_ATTACHMENT0
        };
        glDrawBuffers(1, draw_buffers);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    bool validate() {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        bool result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return result;
    }

    int colorTargetCount() const {
        return color_targets.size();
    }
    const char* getColorTargetName(int i) const {
        return color_targets[i].name.c_str();
    }

    unsigned int getId() const {
        return fbo;
    }
};

inline void gpuFrameBufferBind(gpuFrameBuffer* fb) {
    glBindFramebuffer(GL_FRAMEBUFFER, fb->getId());
}
inline void gpuFrameBufferUnbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

class gpuPipelinePass {
    gpuFrameBuffer* fb = 0;
public:
    void setFrameBuffer(gpuFrameBuffer* fb) {
        this->fb = fb;
    }
    gpuFrameBuffer* getFrameBuffer() {
        return fb;
    }
    void bindFrameBuffer() {
        gpuFrameBufferBind(fb);
    }
};

class gpuPipelineTechnique {
    int id;
    std::vector<gpuPipelinePass> passes;
public:
    gpuPipelineTechnique(int id, int n_passes)
    : id(id) {
        passes.resize(n_passes);
    }
    int getId() const {
        return id;
    }
    gpuPipelinePass* getPass(int i) {
        return &passes[i];
    }
    int passCount() const {
        return passes.size();
    }
};

class gpuPipeline {
    std::vector<std::unique_ptr<gpuPipelineTechnique>> techniques;
    std::map<std::string, gpuPipelineTechnique*> techniques_by_name;

    std::vector<std::unique_ptr<gpuUniformBufferDesc>> uniform_buffer_descs;
    std::map<std::string, gpuUniformBufferDesc*> uniform_buffer_descs_by_name;

    std::vector<std::unique_ptr<gpuUniformBuffer>> uniform_buffers;
    std::vector<std::unique_ptr<gpuRenderMaterial>> materials;

    std::vector<gpuUniformBuffer*> attached_uniform_buffers;
public:
    gpuPipelineTechnique* createTechnique(const char* name, int n_passes) {
        int id = techniques.size();
        techniques.emplace_back(std::unique_ptr<gpuPipelineTechnique>(new gpuPipelineTechnique(id, n_passes)));
        techniques_by_name[name] = techniques.back().get();
        return techniques.back().get();
    }

    gpuUniformBufferDesc* createUniformBufferDesc(const char* name) {
        auto it = uniform_buffer_descs_by_name.find(name);
        if (it != uniform_buffer_descs_by_name.end()) {
            assert(false);
            return 0;
        }
        
        int id = uniform_buffer_descs.size();
        auto ptr = new gpuUniformBufferDesc();
        uniform_buffer_descs.emplace_back(std::unique_ptr<gpuUniformBufferDesc>(ptr));
        uniform_buffer_descs_by_name[name] = uniform_buffer_descs.back().get();
        ptr->name(name);
        ptr->id = id;
        return ptr;
    }
    gpuUniformBufferDesc* getUniformBufferDesc(const char* name) {
        auto it = uniform_buffer_descs_by_name.find(name);
        if (it == uniform_buffer_descs_by_name.end()) {
            return 0;
        }
        return it->second;
    }
    gpuUniformBuffer* createUniformBuffer(const char* name) {
        auto desc = getUniformBufferDesc(name);
        if (!desc) {
            return 0;
        }
        auto ub = new gpuUniformBuffer(desc);
        uniform_buffers.push_back(std::unique_ptr<gpuUniformBuffer>(ub));
        return ub;
    }

    void attachUniformBuffer(gpuUniformBuffer* buf) {
        attached_uniform_buffers.push_back(buf);
    }

    bool compile() {
        return true;
    }

    gpuRenderMaterial* createMaterial() {
        auto ptr = new gpuRenderMaterial(this);
        materials.push_back(std::unique_ptr<gpuRenderMaterial>(ptr));
        return ptr;
    }

    void bindUniformBuffers() {
        for (int i = 0; i < attached_uniform_buffers.size(); ++i) {
            auto& ub = attached_uniform_buffers[i];
            GLint gl_id = ub->gpu_buf.getId();
            glBindBufferBase(GL_UNIFORM_BUFFER, ub->getDesc()->id, gl_id);
        }
    }
    
    int uniformBufferCount() const {
        return uniform_buffer_descs.size();
    }
    int techniqueCount() const {
        return techniques.size();
    }
   
    gpuUniformBufferDesc* getUniformBuffer(int i) {
        return uniform_buffer_descs[i].get();
    }
    gpuPipelinePass* getPass(int tech, int pass) {
        return techniques[tech]->getPass(pass);
    }
    gpuPipelineTechnique* findTechnique(const char* name) {
        auto it = techniques_by_name.find(name);
        if (it == techniques_by_name.end()) {
            return 0;
        }
        return it->second;
    }
};