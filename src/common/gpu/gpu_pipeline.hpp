#pragma once

#include <vector>
#include <memory>
#include <map>
#include "platform/gl/glextutil.h"
#include "gpu/glx_texture_2d.hpp"
#include "gpu/gpu_uniform_buffer.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_render_target.hpp"
#include "gpu/gpu_framebuffer.hpp"
#include "util/strid.hpp"


class gpuPipeline;
class gpuPipelinePass {
    friend gpuPipeline;

    struct ColorTargetDesc {
        std::string local_name;
        std::string global_name;
        int global_index;
    };
    struct DepthTargetDesc {
        std::string global_name;
        int global_index = -1;
    };

    int framebuffer_id = -1;
    std::vector<ColorTargetDesc> color_targets;
    std::map<std::string, int> color_target_map;
    DepthTargetDesc depth_target;

    std::unordered_map<string_id, int> target_sampler_indices;

    gpuFrameBuffer* fb = 0;
public:
    void setFrameBufferId(int id) { framebuffer_id = id; }
    int getFrameBufferId() const { return framebuffer_id; }

    void setTargetSampler(const char* name) {
        // Uninitialized at first
        // indices are filled at pipeline compile() time
        target_sampler_indices.insert(
            std::make_pair(string_id(name), -1)
        );
    }
    int getTargetSamplerTextureIndex(string_id id) {
        auto it = target_sampler_indices.find(id);
        return it->second;
    }
    void setColorTarget(const char* name, const char* global_name) {
        auto it = color_target_map.find(name);
        if (it != color_target_map.end()) {
            assert(false);
            LOG_ERR("Color target " << name << " already exists");
            return;
        }
        color_target_map[name] = color_targets.size();
        ColorTargetDesc desc;
        desc.global_index = -1;
        desc.global_name = global_name;
        desc.local_name = name;
        color_targets.push_back(desc);
    }
    void setDepthTarget(const char* global_name) {
        depth_target.global_name = global_name;
        depth_target.global_index = -1;
    }
    int colorTargetCount() const {
        return color_targets.size();
    }
    bool hasDepthTarget() const {
        return depth_target.global_index != -1;
    }
    const std::string& getColorTargetGlobalName(int idx) const {
        return color_targets[idx].global_name;
    }
    const std::string& getColorTargetLocalName(int idx) const {
        return color_targets[idx].local_name;
    }
    void setColorTargetTextureIndex(int target_idx, int texture_idx) {
        color_targets[target_idx].global_index = texture_idx;
    }
    int getColorTargetTextureIndex(int target_idx) const {
        return color_targets[target_idx].global_index;
    }

    const std::string& getDepthTargetGlobalName() const {
        return depth_target.global_name;
    }
    void setDepthTargetTextureIndex(int texture_idx) {
        depth_target.global_index = texture_idx;
    }
    int getDepthTargetTextureIndex() const {
        return depth_target.global_index;
    }

    /*
    void setFrameBuffer(gpuFrameBuffer* fb) {
        this->fb = fb;
    }
    gpuFrameBuffer* getFrameBuffer() {
        return fb;
    }
    void bindFrameBuffer() {
        gpuFrameBufferBind(fb);
    }*/
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
    int next_framebuffer_index = 0;

    struct RenderTargetLayer {
        std::string name;
        GLint format;
    };/*
    struct DepthRenderTarget {
        std::string name;
    };*/
    std::vector<RenderTargetLayer> render_targets;
    //std::vector<DepthRenderTarget> depth_render_targets;
    std::map<std::string, int> rt_map;
    //std::map<std::string, int> depth_rt_map;

    std::vector<std::unique_ptr<gpuUniformBufferDesc>> uniform_buffer_descs;
    std::map<std::string, gpuUniformBufferDesc*> uniform_buffer_descs_by_name;

    std::vector<std::unique_ptr<gpuUniformBuffer>> uniform_buffers;
    //std::vector<std::unique_ptr<gpuMaterial>> materials;

    std::vector<gpuUniformBuffer*> attached_uniform_buffers;
public:
    virtual ~gpuPipeline() {}

    void addColorRenderTarget(const char* name, GLint format) {
        auto it = rt_map.find(name);
        if (it != rt_map.end()) {
            assert(false);
            LOG_ERR("Color render target " << name << " already exists");
            return;
        }
        int index = render_targets.size();
        render_targets.push_back(RenderTargetLayer{ name, format });
        rt_map[name] = index;
    }
    void addDepthRenderTarget(const char* name) {
        auto it = rt_map.find(name);
        if (it != rt_map.end()) {
            assert(false);
            LOG_ERR("Depth render target " << name << " already exists");
            return;
        }
        int index = render_targets.size();
        render_targets.push_back(RenderTargetLayer{ name, GL_DEPTH_COMPONENT });
        rt_map[name] = index;
    }

    gpuPipelineTechnique* createTechnique(const char* name, int n_passes) {
        int id = techniques.size();
        techniques.emplace_back(std::unique_ptr<gpuPipelineTechnique>(new gpuPipelineTechnique(id, n_passes)));
        techniques_by_name[name] = techniques.back().get();
        for (int i = 0; i < n_passes; ++i) {
            auto pass = techniques.back()->getPass(i);
            pass->setFrameBufferId(next_framebuffer_index + i);
        }
        next_framebuffer_index += n_passes;

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
        for (int i = 0; i < techniques.size(); ++i) {
            auto& tech = techniques[i];
            for (int j = 0; j < tech->passCount(); ++j) {
                auto pass = tech->getPass(j);
                // Color targets
                for (int k = 0; k < pass->colorTargetCount(); ++k) {
                    auto& tgt_name = pass->getColorTargetGlobalName(k);
                    auto it = rt_map.find(tgt_name);
                    if (it == rt_map.end()) {
                        assert(false);
                        LOG_ERR("Color target " << tgt_name << " does not exist");
                        pass->setColorTargetTextureIndex(k, -1);
                        continue;
                    }
                    pass->setColorTargetTextureIndex(k, it->second);
                }
                // Depth target
                auto& tgt_name = pass->getDepthTargetGlobalName();
                if (!tgt_name.empty()) {                    
                    auto it = rt_map.find(tgt_name);
                    if (it == rt_map.end()) {
                        assert(false);
                        LOG_ERR("Depth target " << tgt_name << " does not exist");
                        pass->setDepthTargetTextureIndex(-1);
                        continue;
                    }
                    pass->setDepthTargetTextureIndex(it->second);
                }
                // Target samplers
                for (auto& kv : pass->target_sampler_indices) {
                    auto it = rt_map.find(kv.first.to_string());
                    if (it == rt_map.end()) {
                        assert(false);
                        LOG_ERR("Render target layer " << kv.first.to_string() << " does not exist");
                        continue;
                    }
                    kv.second = it->second;
                }
            }
        }

        return true;
    }

    void initRenderTarget(gpuRenderTarget* rt) {
        rt->pipeline = this;
        rt->setSize(800, 600);

        for (int i = 0; i < render_targets.size(); ++i) {
            auto rtdesc = render_targets[i];
            rt->textures.push_back(
                HSHARED<gpuTexture2d>(HANDLE_MGR<gpuTexture2d>::acquire())
            );
            // TODO: DERIVE CHANNEL COUNT FROM FORMAT
            if (rtdesc.format == GL_RGB) {
                rt->textures.back()->changeFormat(rtdesc.format, 800, 600, 3);
            } else if(rtdesc.format == GL_DEPTH_COMPONENT) {
                rt->textures.back()->changeFormat(rtdesc.format, 800, 600, 1);
            } else {
                assert(false);
            }
        }

        for (int i = 0; i < techniques.size(); ++i) {
            auto& tech = techniques[i];
            for (int j = 0; j < tech->passCount(); ++j) {
                auto pass = tech->getPass(j);
                auto fb = new gpuFrameBuffer;
                rt->framebuffers.push_back(std::unique_ptr<gpuFrameBuffer>(fb));
                for (int k = 0; k < pass->colorTargetCount(); ++k) {
                    fb->addColorTarget(
                        pass->getColorTargetLocalName(k).c_str(),
                        rt->textures[pass->getColorTargetTextureIndex(k)].get()
                    );
                }
                if (pass->hasDepthTarget()) {
                    fb->addDepthTarget(
                        rt->textures[pass->getDepthTargetTextureIndex()]
                    );
                }
                if (!fb->validate()) {
                    assert(false);
                    LOG_ERR("FrameBuffer validation failed");
                    continue;
                }
                fb->prepare();
            }
        }
    }

    int getTargetLayerIndex(const char* name) {
        auto it = rt_map.find(name);
        if (it == rt_map.end()) {
            assert(false);
            LOG_ERR("getTargetLayerIndex(): " << name << " does not exist");
            return -1;
        }
        return it->second;
    }
    int getFrameBufferIndex(const char* technique, int pass) {
        auto it_tech = techniques_by_name.find(technique);
        if (it_tech == techniques_by_name.end()) {
            assert(false);
            LOG_ERR("Can't find technique " << technique);
            return -1;
        }
        auto tech = it_tech->second;
        auto p = tech->getPass(pass);
        return p->getFrameBufferId();
    }

    gpuMaterial* createMaterial() {
        auto ptr = new gpuMaterial();
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
    gpuPipelineTechnique* getTechnique(int i) {
        return techniques[i].get();
    }
    gpuPipelineTechnique* findTechnique(const char* name) {
        auto it = techniques_by_name.find(name);
        if (it == techniques_by_name.end()) {
            return 0;
        }
        return it->second;
    }
};