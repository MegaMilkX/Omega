#pragma once

#include "gpu/gpu_render_target.hpp"
#include "gpu/gpu_material.hpp"
#include "util/strid.hpp"

struct DRAW_PARAMS {
    gfxm::mat4 view = gfxm::mat4(1.f);
    gfxm::mat4 projection = gfxm::mat4(1.f);
    int viewport_x = 0;
    int viewport_y = 0;
    int viewport_width = 0;
    int viewport_height = 0;
};

class gpuPipeline;
class gpuRenderBucket;
class gpuPass {
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

    std::map<std::string, int> color_target_map;
    std::vector<ColorTargetDesc> color_targets;
    DepthTargetDesc depth_target;
    std::unordered_map<string_id, int> target_sampler_indices;
protected:
    int framebuffer_id = -1;

public:
    virtual ~gpuPass() {}

    int getFrameBufferId() const { return framebuffer_id; }
    gpuPass* setColorTarget(const char* name, const char* global_name) {
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
    gpuPass* setDepthTarget(const char* global_name) {
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
    int getDepthTargetTextureIndex() const {
        return depth_target.global_index;
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
    gpuPass* setTargetSampler(const char* name) {
        // Uninitialized at first
        // indices are filled at pipeline compile() time
        target_sampler_indices.insert(
            std::make_pair(string_id(name), -1)
        );
        return this;
    }
    int getTargetSamplerTextureIndex(string_id id) {
        auto it = target_sampler_indices.find(id);
        return it->second;
    }

    virtual void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id, const DRAW_PARAMS& params) {}
};