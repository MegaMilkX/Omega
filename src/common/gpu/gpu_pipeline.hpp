#pragma once

#include <vector>
#include <memory>
#include <map>
#include "platform/gl/glextutil.h"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_uniform_buffer.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_render_target.hpp"
#include "gpu/gpu_framebuffer.hpp"
#include "gpu/pass/gpu_pass.hpp"
#include "util/strid.hpp"

/*
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

    gpuPipelinePass* setTargetSampler(const char* name) {
        // Uninitialized at first
        // indices are filled at pipeline compile() time
        target_sampler_indices.insert(
            std::make_pair(string_id(name), -1)
        );
        return this;
    }
    int getColorSourceTextureIndex(string_id id) {
        auto it = target_sampler_indices.find(id);
        return it->second;
    }
    gpuPipelinePass* setColorTarget(const char* name, const char* global_name) {
        auto it = color_target_map.find(name);
        if (it != color_target_map.end()) {
            assert(false);
            LOG_ERR("Color target " << name << " already exists");
            return this;
        }
        color_target_map[name] = color_targets.size();
        ColorTargetDesc desc;
        desc.global_index = -1;
        desc.global_name = global_name;
        desc.local_name = name;
        color_targets.push_back(desc);
        return this;
    }
    gpuPipelinePass* setDepthTarget(const char* global_name) {
        depth_target.global_name = global_name;
        depth_target.global_index = -1;
        return this;
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
};*/

class gpuPipelineTechnique {
    int id;
    bool is_enabled = true;
    std::vector<gpuPass*> passes;
public:
    gpuPipelineTechnique(int id, bool dont_execute)
    : id(id), is_enabled(!dont_execute) {}

    bool isEnabled() const {
        return is_enabled;
    }

    int getId() const {
        return id;
    }
    void addPass(gpuPass* pass) {
        passes.push_back(pass);
    }
    gpuPass* getPass(int i) {
        return passes[i];
    }
    int passCount() const {
        return passes.size();
    }
};

class gpuPipeline {
public:
    struct RenderChannel {
        std::string name;
        GLint format;
        int lwt;
        bool is_depth;
        bool is_double_buffered;
        gfxm::vec3 clear_color;
    };

private:
    std::vector<std::unique_ptr<gpuPipelineTechnique>> techniques;
    std::map<std::string, gpuPipelineTechnique*> techniques_by_name;
    int next_framebuffer_index = 0;

    std::vector<RenderChannel> render_channels;
    std::map<std::string, int> rt_map;
    int output_target = -1;

    std::vector<std::unique_ptr<gpuUniformBufferDesc>> uniform_buffer_descs;
    std::map<std::string, gpuUniformBufferDesc*> uniform_buffer_descs_by_name;

    std::vector<std::unique_ptr<gpuUniformBuffer>> uniform_buffers;

    std::vector<gpuUniformBuffer*> attached_uniform_buffers;
public:
    virtual ~gpuPipeline() {}

    virtual void init() = 0;

    void addColorChannel(
        const char* name,
        GLint format,
        bool is_double_buffered = false,
        const gfxm::vec4& clear_color = gfxm::vec4(0, 0, 0, 0)
    ) {
        auto it = rt_map.find(name);
        if (it != rt_map.end()) {
            assert(false);
            LOG_ERR("Color render target " << name << " already exists");
            return;
        }
        int index = render_channels.size();
        render_channels.push_back(RenderChannel{
            .name = name,
            .format = format,
            .lwt = 0,
            .is_depth = false,
            .is_double_buffered = is_double_buffered,
            .clear_color = clear_color
        });
        rt_map[name] = index;
    }
    void addDepthChannel(const char* name) {
        auto it = rt_map.find(name);
        if (it != rt_map.end()) {
            assert(false);
            LOG_ERR("Depth render target " << name << " already exists");
            return;
        }
        int index = render_channels.size();
        render_channels.push_back(RenderChannel{
            .name = name,
            .format = GL_DEPTH_COMPONENT,
            .lwt = 0,
            .is_depth = true,
            .is_double_buffered = false
        });
        rt_map[name] = index;
    }
    void setOutputChannel(const char* render_target_name) {
        auto it = rt_map.find(render_target_name);
        if (it == rt_map.end()) {
            LOG_ERR("setOutputSource(): render target '" << render_target_name << "' does not exist");
            assert(false);
            return;
        }
        output_target = it->second;
    }

    gpuPipelineTechnique* createTechnique(const char* name, bool dont_execute = false) {
        int id = techniques.size();
        techniques.emplace_back(std::unique_ptr<gpuPipelineTechnique>(new gpuPipelineTechnique(id, dont_execute)));
        techniques_by_name[name] = techniques.back().get();

        return techniques.back().get();
    }
    gpuPass* addPass(gpuPipelineTechnique* tech, gpuPass* pass) {
        pass->framebuffer_id = next_framebuffer_index;
        //pass->setFrameBufferId(next_framebuffer_index);
        ++next_framebuffer_index;
        tech->addPass(pass);
        return pass;
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
    void destroyUniformBuffer(gpuUniformBuffer* buf) {
        for (int i = 0; i < uniform_buffers.size(); ++i) {
            if (uniform_buffers[i].get() == buf) {
                uniform_buffers.erase(uniform_buffers.begin() + i);
                break;
            }
        }
    }

    void attachUniformBuffer(gpuUniformBuffer* buf) {
        attached_uniform_buffers.push_back(buf);
    }

    bool compile() {
        for (int i = 0; i < channelCount(); ++i) {
            auto channel = getChannel(i);
            channel->lwt = 0;
        }

        for (int i = 0; i < techniques.size(); ++i) {
            auto& tech = techniques[i];
            for (int j = 0; j < tech->passCount(); ++j) {
                auto pass = tech->getPass(j);

                // Set render target channel indices
                for (int k = 0; k < pass->channelCount(); ++k) {
                    gpuPass::ChannelDesc* ch_desc = pass->getChannelDesc(k);
                    const std::string& ch_name = ch_desc->pipeline_channel_name;
                    auto it = rt_map.find(ch_name);
                    if (it == rt_map.end()) {
                        assert(false);
                        LOG_ERR("Pipeline channel '" << ch_name << "' does not exist");
                        ch_desc->render_target_channel_idx = -1;
                        continue;
                    }
                    ch_desc->render_target_channel_idx = it->second;
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

                {
                    for (int k = 0; k < pass->channelCount(); ++k) {
                        gpuPass::ChannelDesc* ch_desc = pass->getChannelDesc(k);
                        const RenderChannel* pipeline_channel = getChannel(ch_desc->render_target_channel_idx);

                        int& lwt = getChannel(ch_desc->render_target_channel_idx)->lwt;
                        ch_desc->lwt_buffer_idx = lwt;
                        
                        if (!pipeline_channel->is_double_buffered) {
                            continue;
                        }

                        if (ch_desc->reads && ch_desc->writes) {
                            lwt = (lwt + 1) % 2;
                        }
                    }
                }

                // Shader sampler sets
                for (int k = 0; k < pass->shaderCount(); ++k) {
                    const gpuShaderProgram* shader = pass->getShader(k);
                    
                    ShaderSamplerSet* sampler_set = pass->getSamplerSet(k);
                    sampler_set->clear();
                    for (int l = 0; l < pass->textureCount(); ++l) {
                        auto tex_desc = pass->getTextureDesc(l);
                        int slot = shader->getDefaultSamplerSlot(tex_desc->sampler_name.c_str());
                        if (slot < 0) {
                            continue;
                        }

                        ShaderSamplerSet::Sampler sampler;
                        sampler.source = SHADER_SAMPLER_SOURCE_GPU;
                        sampler.type = tex_desc->type;
                        sampler.slot = slot;
                        sampler.texture_id = tex_desc->texture;
                        sampler_set->add(sampler);
                    }

                    for (int l = 0; l < pass->channelCount(); ++l) {
                        const gpuPass::ChannelDesc* ch_desc = pass->getChannelDesc(l);
                        if (!ch_desc->reads) {
                            continue;
                        }
                        const std::string& ch_name = ch_desc->source_local_name;

                        // TODO: Use a prefix for glsl sampler names
                        int slot = shader->getDefaultSamplerSlot(ch_name.c_str());
                        if (slot < 0) {
                            continue;
                        }

                        ShaderSamplerSet::Sampler sampler;
                        sampler.source = SHADER_SAMPLER_SOURCE_CHANNEL_IDX;
                        sampler.type = SHADER_SAMPLER_TEXTURE2D;
                        sampler.slot = slot;
                        sampler.channel_idx
                            = ShaderSamplerSet::ChannelBufferIdx{ ch_desc->render_target_channel_idx, ch_desc->lwt_buffer_idx };
                        sampler_set->add(sampler);
                    }
                }
            }
        }

        return true;
    }

    void initRenderTarget(gpuRenderTarget* rt) {
        LOG("Initializing render target");
        rt->pipeline = this;
        rt->default_output_texture = output_target;

        LOG("Creating render target textures");
        for (int i = 0; i < render_channels.size(); ++i) {
            auto rtdesc = render_channels[i];

            gpuRenderTarget::TextureLayer layer;
            layer.textures[0] = HSHARED<gpuTexture2d>(HANDLE_MGR<gpuTexture2d>::acquire());
            if (rtdesc.is_double_buffered) {
                layer.textures[1] = HSHARED<gpuTexture2d>(HANDLE_MGR<gpuTexture2d>::acquire());
            }

            /*rt->textures.push_back(
                HSHARED<gpuTexture2d>(HANDLE_MGR<gpuTexture2d>::acquire())
            );*/
            // TODO: DERIVE CHANNEL COUNT FROM FORMAT
            if (rtdesc.format == GL_RGB) {
                layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 3);
                layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
                if (rtdesc.is_double_buffered) {
                    layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 3);
                    layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
                }
            } else if (rtdesc.format == GL_SRGB) {
                layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 3);
                layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
                if (rtdesc.is_double_buffered) {
                    layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 3);
                    layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
                }
            } else if(rtdesc.format == GL_RED) {
                layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 1);
                layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
                if (rtdesc.is_double_buffered) {
                    layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 1);
                    layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
                }
            } else if(rtdesc.format == GL_RGB32F) {
                layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 3, GL_FLOAT);
                layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
                if (rtdesc.is_double_buffered) {
                    layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 3, GL_FLOAT);
                    layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
                }
            } else if(rtdesc.format == GL_RGBA32F) {
                layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 4, GL_FLOAT);
                layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
                if (rtdesc.is_double_buffered) {
                    layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 4, GL_FLOAT);
                    layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
                }
            } else if(rtdesc.format == GL_DEPTH_COMPONENT) {
                layer.textures[0]->changeFormat(rtdesc.format, rt->width, rt->height, 1);
                layer.textures[0]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
                if (rtdesc.is_double_buffered) {
                    layer.textures[1]->changeFormat(rtdesc.format, rt->width, rt->height, 1);
                    layer.textures[1]->setWrapMode(GPU_TEXTURE_WRAP_CLAMP);
                }
            } else {
                assert(false);
                LOG_ERR("Render target format not supported!");
            }
            if (rtdesc.is_depth) {
                if (rtdesc.is_double_buffered) {
                    assert(false);
                    LOG_ERR("!!! Depth texture cannot be double buffered !!!");
                }
                rt->depth_texture = layer.textures[0].get();
            }
            rt->layers.push_back(layer);
        }

        LOG("Creating framebuffers");
        // TODO: DOUBLE BUFFERED RT LAYERS
        // READ + WRITE = read from the last written to, WRITE becomes lwt (last written to)
        // WRITE = write to the last written to, lwt does not change
        // READ = read from the last written to, lwt does not change
        
        //std::vector<int> lwt_data(rt->layers.size()); // Last written to indices, for double buffered layers
        //std::fill(lwt_data.begin(), lwt_data.end(), 0);
        for (int i = 0; i < techniques.size(); ++i) {
            auto& tech = techniques[i];
            for (int j = 0; j < tech->passCount(); ++j) {
                auto pass = tech->getPass(j);

                auto fb = new gpuFrameBuffer;
                rt->framebuffers.push_back(std::unique_ptr<gpuFrameBuffer>(fb));
                
                for (int k = 0; k < pass->channelCount(); ++k) {
                    const gpuPass::ChannelDesc* ch_desc = pass->getChannelDesc(k);
                    const std::string& ch_name = ch_desc->pipeline_channel_name;
                    const gpuPipeline::RenderChannel* pipeline_channel = rt->getPipeline()->getChannel(ch_desc->render_target_channel_idx);
                    /* const */ gpuRenderTarget::TextureLayer& rt_layer = rt->layers[ch_desc->render_target_channel_idx];

                    if (!ch_desc->writes) {
                        continue;
                    }

                    if (pass->hasFlags(PASS_FLAG_CLEAR_PASS)) {
                        assert(!ch_desc->target_local_name.empty());

                        if (pipeline_channel->is_double_buffered) {
                            // NOTE: two addColorTarget() with same name
                            // is ok (for now) since we do not use those names
                            // to retrieve buffers, but retrieve names using indices
                            fb->addColorTarget(
                                std::format("{}{}", ch_desc->target_local_name, 0).c_str(),
                                rt_layer.textures[0].get()
                            );
                            fb->addColorTarget(
                                std::format("{}{}", ch_desc->target_local_name, 1).c_str(),
                                rt_layer.textures[1].get()
                            );
                        } else {
                            fb->addColorTarget(
                                ch_desc->target_local_name.c_str(),
                                rt_layer.textures[0].get()
                            );
                        }
                    } else if (ch_desc->reads && ch_desc->writes) {
                        assert(!ch_desc->target_local_name.empty());
                        if (!pipeline_channel->is_double_buffered) {
                            assert(false);
                            LOG_ERR("Misconfig: Render target layer '" << ch_name << "' is not double buffered, but a pass tries to use it as such");
                            continue;
                        }

                        fb->addColorTarget(
                            ch_desc->target_local_name.c_str(),
                            rt_layer.textures[(ch_desc->lwt_buffer_idx + 1) % 2].get()
                        );
                    } else if(ch_desc->writes) {
                        assert(!ch_desc->target_local_name.empty());
                        fb->addColorTarget(
                            ch_desc->target_local_name.c_str(),
                            rt_layer.textures[ch_desc->lwt_buffer_idx].get()
                        );
                    }
                }
                /*
                for (auto& kv : io_data) {
                    const std::string& name = kv.first;
                    const PassColorIO& io = kv.second;

                    gpuRenderTarget::TextureLayer& rt_layer = rt->layers[io.rt_layer_idx];
                    int* lwt = &lwt_data[io.rt_layer_idx];
                    
                    if (io.reads && io.writes) {
                        if (!rt_layer.is_double_buffered) {
                            assert(false);
                            LOG_ERR("Misconfig: Render target layer '" << name << "' is not double buffered, but a pass tries to use it as such");
                            continue;
                        }
                        gpuTexture2d* buffers[2] = {
                            rt_layer.texture_a.get(), rt_layer.texture_b.get()
                        };
                        fb->addColorTarget(io.local_target_name.c_str(), buffers[(*lwt + 1) % 2]);
                    } else if(io.reads) {
                    
                    } else if(io.writes) {
                    
                    } else {
                        // Should not be possible ever
                        assert(false);
                        LOG_ERR("initRenderTarget: Unreachable code");
                    }
                }*/
                /*
                for (int k = 0; k < pass->colorTargetCount(); ++k) {
                    int layer_idx = pass->getColorTargetTextureIndex(k);
                    fb->addColorTarget(
                        pass->getColorTargetLocalName(k).c_str(),
                        rt->textures[layer_idx].get()
                    );
                }*/

                if (pass->hasDepthTarget()) {
                    fb->addDepthTarget(
                        rt->layers[pass->getDepthTargetTextureIndex()].textures[0]
                    );
                }

                if (!fb->validate()) {
                    assert(false);
                    LOG_ERR("FrameBuffer validation failed");
                    continue;
                }
                //fb->prepare();
            }
        }
        LOG("Render target initialized");
    }

    int channelCount() const {
        return render_channels.size();
    }
    RenderChannel* getChannel(int i) {
        return &render_channels[i];
    }
    const RenderChannel* getChannel(int i) const {
        return &render_channels[i];
    }
    int getChannelIndex(const char* name) {
        auto it = rt_map.find(name);
        if (it == rt_map.end()) {
            assert(false);
            LOG_ERR("getChannelIndex(): " << name << " does not exist");
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
        return p->framebuffer_id;
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
    gpuPass* getPass(int tech, int pass) {
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