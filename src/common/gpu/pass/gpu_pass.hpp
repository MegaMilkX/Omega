#pragma once

#include "gpu/gpu_render_target.hpp"
#include "gpu/gpu_material.hpp"
#include "util/strid.hpp"

typedef uint32_t pass_flags_t;
constexpr pass_flags_t PASS_FLAG_NONE = 0x00;
constexpr pass_flags_t PASS_FLAG_CLEAR_PASS = 0x01; // Framebuffer for this pass will be populated with both buffers for double buffered channels
constexpr pass_flags_t PASS_FLAG_NO_DRAW = 0x02; // Skip pass during normal drawing
constexpr pass_flags_t PASS_FLAG_DISABLED = 0x04;

struct DRAW_PARAMS {
    gfxm::mat4 view = gfxm::mat4(1.f);
    gfxm::mat4 projection = gfxm::mat4(1.f);
    gfxm::rect vp_rect_ratio;
    int viewport_x = 0;
    int viewport_y = 0;
    int viewport_width = 0;
    int viewport_height = 0;
    float time = .0f;
};

class gpuPipeline;
class gpuRenderBucket;

class gpuPass {
    friend gpuPipeline;

    struct DepthTargetDesc {
        std::string global_name;
        int global_index = -1;
    };
    struct SamplerSlotFrameImagePair {
        int sampler_slot;
        int channel_idx;
    };
public:
    struct ChannelDesc {
        std::string pipeline_channel_name;
        std::string source_local_name;
        std::string target_local_name;
        int render_target_channel_idx = -1;
        int lwt_buffer_idx = 0;
        bool reads = false;
        bool writes = false;
    };
    struct TextureDesc {
        std::string sampler_name;
        GLuint texture;
        SHADER_SAMPLER_TYPE type;
    };

private:
    int id = 0;
    pass_flags_t flags;

    DepthTargetDesc depth_target;

    std::vector<RHSHARED<gpuShaderProgram>> shaders;
    std::vector<ShaderSamplerSet> sampler_sets;

    std::vector<TextureDesc> textures;

    std::vector<ChannelDesc> channels;
    std::map<std::string, int> channels_by_name;

    // Compiled
    std::vector<SamplerSlotFrameImagePair> sampler_slot_frame_image_pairs;

protected:
    int framebuffer_id = -1;

    gpuShaderProgram* addShader(const RHSHARED<gpuShaderProgram>& shader) {
        shaders.push_back(shader);
        sampler_sets.resize(shaders.size());
        return shaders.back().get();
    }
    ShaderSamplerSet* getSamplerSet(int i) {
        return &sampler_sets[i];
    }

    void addTexture(const char* sampler_name, GLuint texture, SHADER_SAMPLER_TYPE type = SHADER_SAMPLER_TEXTURE2D) {
        textures.push_back(
            TextureDesc{
                .sampler_name = sampler_name,
                .texture = texture,
                .type = type
            }
        );
    }
    int textureCount() const {
        return textures.size();
    }
    const TextureDesc* getTextureDesc(int i) const {
        return &textures[i];
    }

    void bindFramebuffer(gpuRenderTarget* target) {
        if (framebuffer_id < 0) {
            assert(false);
            return;
        }
        gpuFrameBufferBind(target->framebuffers[framebuffer_id].get());
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            assert(false);
            return;
        }
    }
    void bindDrawBuffers(gpuRenderTarget* target) {
        assert(
            target->framebuffers[framebuffer_id]->colorTargetCount() <=
            GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS
        );

        GLenum draw_buffers[GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS];
        for (int i = 0; i < GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS; ++i) {
            draw_buffers[i] = GL_COLOR_ATTACHMENT0 + i;
        }
        glDrawBuffers(
            gfxm::_min(
                GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS,
                target->framebuffers[framebuffer_id]->colorTargetCount()
            ),
            draw_buffers
        );
    }

public:
    gpuPass(pass_flags_t flags = PASS_FLAG_NONE)
    : flags(flags) {}
    virtual ~gpuPass() {}

    void enable(bool value) {
        if(value) {
            removeFlags(PASS_FLAG_DISABLED);
        } else {
            addFlags(PASS_FLAG_DISABLED);
        }
    }

    int getId() const { return id; }
    void addFlags(pass_flags_t fl) { flags |= fl; }
    void removeFlags(pass_flags_t fl) { flags &= ~fl; }
    pass_flags_t getFlags() const { return flags; }
    bool hasFlags(pass_flags_t fl) { return (flags & fl) == fl; }
    bool hasAnyFlags(pass_flags_t fl) { return (flags & fl) != 0; }

    int shaderCount() const {
        return (int)shaders.size();
    }
    const gpuShaderProgram* getShader(int i) const {
        return shaders[i].get();
    }

    ChannelDesc* getChannelDesc(const std::string& name) {
        auto it = channels_by_name.find(name);
        if (it == channels_by_name.end()) {
            return 0;
        }
        return &channels[it->second];
    }
    const ChannelDesc* getChannelDesc(const std::string& name) const {
        auto it = channels_by_name.find(name);
        if (it == channels_by_name.end()) {
            return 0;
        }
        return &channels[it->second];
    }
    ChannelDesc* getChannelDesc(int i) {
        return &channels[i];
    }
    const ChannelDesc* getChannelDesc(int i) const {
        return &channels[i];
    }

    int getFrameBufferId() const { return framebuffer_id; }
    
    int channelCount() const {
        return channels.size();
    }
    
    gpuPass* setColorTarget(const char* name, const char* global_name) {
        auto it = channels_by_name.find(global_name);
        if (it == channels_by_name.end()) {
            it = channels_by_name.insert(std::make_pair(std::string(global_name), channels.size())).first;
            channels.push_back(ChannelDesc());
        }
        ChannelDesc& desc = channels[it->second];
        desc.writes = true;
        desc.render_target_channel_idx = -1;
        desc.pipeline_channel_name = global_name;
        desc.target_local_name = name;
        return this;
        /*auto it = color_target_map.find(name);
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
        return this;*/
    }
    /*
    int colorTargetCount() const {
        return color_targets.size();
    }*/
    const std::string& getColorTargetGlobalName(int idx) const {
        return channels[idx].pipeline_channel_name;
    }
    const std::string& getColorTargetLocalName(int idx) const {
        return channels[idx].target_local_name;
    }
    void setColorTargetTextureIndex(int target_idx, int texture_idx) {
        channels[target_idx].render_target_channel_idx = texture_idx;
        //color_targets[target_idx].global_index = texture_idx;
    }
    int getColorTargetTextureIndex(int target_idx) const {
        return channels[target_idx].render_target_channel_idx;
        //return color_targets[target_idx].global_index;
    }

    gpuPass* setDepthTarget(const char* global_name) {
        depth_target.global_name = global_name;
        depth_target.global_index = -1;
        return this;
    }
    bool hasDepthTarget() const {
        return depth_target.global_index != -1;
    }
    int getDepthTargetTextureIndex() const {
        return depth_target.global_index;
    }
    const std::string& getDepthTargetGlobalName() const {
        return depth_target.global_name;
    }
    void setDepthTargetTextureIndex(int texture_idx) {
        depth_target.global_index = texture_idx;
    }
    
    gpuPass* addColorSource(const char* shader_sampler_name, const char* channel_name) {
        auto it = channels_by_name.find(channel_name);
        if (it == channels_by_name.end()) {
            it = channels_by_name.insert(std::make_pair(std::string(channel_name), channels.size())).first;
            channels.push_back(ChannelDesc());
        }
        ChannelDesc& desc = channels[it->second];
        desc.reads = true;
        desc.render_target_channel_idx = -1;
        desc.pipeline_channel_name = channel_name;
        desc.source_local_name = shader_sampler_name;
        return this;
        /*
        // Uninitialized at first
        // indices are filled at pipeline compile() time
        target_sampler_indices.insert(
            std::make_pair(string_id(frame_image_name), -1)
        );
        return this;*/
    }
    /*
    int colorSourceCount() const {
        return target_sampler_indices.size();
    }*/
    const std::string& getColorSourcePipelineName(int i) const {
        return channels[i].pipeline_channel_name;/*
        auto it = target_sampler_indices.begin();
        std::advance(it, i);
        return it->first;*/
    }
    // TODO: Remove this
    int getColorSourceTextureIndex(string_id id) {
        auto it = channels_by_name.find(id.to_string());
        if (it == channels_by_name.end()) {
            return -1;
        }
        return channels[it->second].render_target_channel_idx;
        /*auto it = target_sampler_indices.find(id);
        return it->second;*/
    }
    int getColorSourceTextureIndex(int i) const {
        return channels[i].render_target_channel_idx;
        /*auto it = target_sampler_indices.begin();
        std::advance(it, i);
        return it->second;*/
    }    

    virtual void onCompiled(gpuPipeline* pipeline) {}
    virtual void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int pass_id, const DRAW_PARAMS& params) {}
};