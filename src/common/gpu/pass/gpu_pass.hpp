#pragma once

#include "gpu/gpu_types.hpp"
#include "gpu/types.hpp"
#include "gpu/render_target_map.hpp"
#include "gpu/gpu_material.hpp"
#include "util/strid.hpp"
#include "platform/platform.hpp"
#include "gpu/render_cmd.hpp"
#include "gpu/gpu_util.hpp"


typedef uint32_t pass_flags_t;
constexpr pass_flags_t PASS_FLAG_NONE = 0x00;
constexpr pass_flags_t PASS_FLAG_CLEAR_PASS = 0x01; // Framebuffer for this pass will be populated with both buffers for double buffered channels
constexpr pass_flags_t PASS_FLAG_NO_DRAW = 0x02; // Skip pass during normal drawing
constexpr pass_flags_t PASS_FLAG_DISABLED = 0x04;

class gpuPipeline;
class gpuRenderBucket;
class gpuRenderCmd;

struct gpuPassInstance {
    gpuPass* pass = nullptr;

    struct ChannelDesc {
        std::string name;
        int16_t render_target_channel_idx = -1;
        int16_t lwt_buffer_idx = -1;
    };
    std::vector<ChannelDesc> channels;
    std::string depth_layer_name;
    int depth_target_idx = -1;
    int framebuffer_id = -1;
    std::vector<int> rt_chan_to_pass;

    gpuPassInstance(gpuPass* pass);
};

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
        int pipe_channel_idx = -1;
        bool reads = false;
        bool writes = false;
    };
    struct TextureDesc {
        std::string sampler_name;
        GLuint texture;
        SHADER_SAMPLER_TYPE type;
    };

private:
    pipe_pass_id_t id = 0;
    std::string full_name;
    pass_flags_t flags;
    GPU_SORT_MODE sort_mode = GPU_SORT_MODE::STATE_CHANGE;
    bool disable_auto_targets = false;

    DepthTargetDesc depth_target;

    /*
    std::vector<RHSHARED<gpuShaderProgram>> shaders;
    std::vector<ShaderSamplerSet> sampler_sets;
    */
    std::vector<ResourceRef<gpuShaderSet>> base_shader_sets;
    RHSHARED<gpuShaderProgram> default_program;
    GLenum default_draw_buffers[GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS];
    ShaderSamplerSet sampler_set;

    std::vector<TextureDesc> textures;

    std::vector<ChannelDesc> channels;
    std::map<std::string, int> channels_by_name;

    // Compiled
    std::vector<SamplerSlotFrameImagePair> sampler_slot_frame_image_pairs;

protected:
    GPU_BLEND_MODE blend_mode = GPU_BLEND_MODE::BLEND;

    void addBaseShaderSet(const ResourceRef<gpuShaderSet>& shaders);
    gpuShaderProgram* getProgram();

    void addTexture(const char* sampler_name, GLuint texture, SHADER_SAMPLER_TYPE type = SHADER_SAMPLER_TEXTURE2D) {
        textures.push_back(
            TextureDesc{
                .sampler_name = sampler_name,
                .texture = texture,
                .type = type
            }
        );
    }

    void bindFramebuffer(gpuPassInstance* inst, gpuRenderTargetMap* target_map) {
        if (inst->framebuffer_id < 0) {
            assert(false);
            return;
        }
        gpuFrameBufferBind(target_map->getFrameBuffer(inst->framebuffer_id));
    }
    void bindDrawBuffers(gpuPassInstance* inst, gpuRenderTargetMap* target_map) {
        assert(
            target_map->getFrameBuffer(inst->framebuffer_id)->colorTargetCount() <=
            GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS
        );

        GLenum draw_buffers[GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS];
        for (int i = 0; i < GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS; ++i) {
            draw_buffers[i] = GL_COLOR_ATTACHMENT0 + i;
        }
        glDrawBuffers(
            gfxm::_min(
                GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS,
                target_map->getFrameBuffer(inst->framebuffer_id)->colorTargetCount()
            ),
            draw_buffers
        );
    }
    void bindDefaultSamplerSet(const gpuRenderTarget* tgt, gpuPassInstance* inst) {
        gpuBindSamplers(tgt, inst, &sampler_set);
    }
    void bindDefaultProgram() {
        glDrawBuffers(GPU_FRAME_BUFFER_MAX_DRAW_COLOR_BUFFERS, default_draw_buffers);
        glUseProgram(default_program->getId());
    }

    gpuShaderProgram* getDefaultProgram() { return default_program.get(); }

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

    pipe_pass_id_t getId() const { return id; }
    void addFlags(pass_flags_t fl) { flags |= fl; }
    void removeFlags(pass_flags_t fl) { flags &= ~fl; }
    pass_flags_t getFlags() const { return flags; }
    bool hasFlags(pass_flags_t fl) { return (flags & fl) == fl; }
    bool hasAnyFlags(pass_flags_t fl) { return (flags & fl) != 0; }

    int textureCount() const {
        return textures.size();
    }
    const TextureDesc* getTextureDesc(int i) const {
        return &textures[i];
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
    const ChannelDesc* getChannelDescByShaderTargetName(const std::string& name) const {
        for (int i = 0; i < channels.size(); ++i) {
            if (channels[i].target_local_name == name) {
                return &channels[i];
            }
        }
        return nullptr;
    }
    ChannelDesc* getChannelDesc(int i) {
        return &channels[i];
    }
    const ChannelDesc* getChannelDesc(int i) const {
        return &channels[i];
    }
    
    int channelCount() const {
        return channels.size();
    }

    std::span<ResourceRef<gpuShaderSet>> getBaseShaderSets() {
        return base_shader_sets;
    }

    gpuPass* setSortMode(GPU_SORT_MODE mode) {
        sort_mode = mode;
        return this;
    }
    gpuPass* setBlending(GPU_BLEND_MODE mode) {
        blend_mode = mode;
        return this;
    }

    gpuPass* disableAutoTargets() {
        disable_auto_targets = true;
    }
    
    gpuPass* setColorTarget(const char* name, const char* global_name) {
        // TODO: don't like counting this every time
        int color_target_count = 0;
        for (int i = 0; i < channels.size(); ++i) {
            if (!channels[i].writes) {
                continue;
            }
            ++color_target_count;
        }
        if (color_target_count == platformGeti(PLATFORM_MAX_FRAMEBUFFER_COLOR_LAYERS)) {
            LOG_ERR("setColorTarget(): too many color targets: " << name << "(" << global_name << ")");
            assert(false);
            return this;
        }

        auto it = channels_by_name.find(global_name);
        if (it == channels_by_name.end()) {
            it = channels_by_name.insert(std::make_pair(std::string(global_name), channels.size())).first;
            channels.push_back(ChannelDesc());
        }
        ChannelDesc& desc = channels[it->second];
        desc.writes = true;
        desc.pipeline_channel_name = global_name;
        desc.target_local_name = name;
        return this;
    }
    const std::string& getColorTargetGlobalName(int idx) const {
        return channels[idx].pipeline_channel_name;
    }
    const std::string& getColorTargetLocalName(int idx) const {
        return channels[idx].target_local_name;
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
        desc.pipeline_channel_name = channel_name;
        desc.source_local_name = shader_sampler_name;
        return this;
    }
    const std::string& getColorSourcePipelineName(int i) const {
        return channels[i].pipeline_channel_name;
    }

    void sortCommands(gpuRenderCmd* commands, size_t count, const DRAW_PARAMS& params);

    virtual void onCompiled(gpuPipeline* pipeline) {}
    virtual void onDraw(gpuPassInstance* inst, gpuRenderTargetMap* target_map, gpuRenderBucket* bucket, pipe_pass_id_t pass_id, const DRAW_PARAMS& params) {}
};

inline gpuPassInstance::gpuPassInstance(gpuPass* pass)
    : pass(pass) {
    channels.resize(pass->channelCount());
}


