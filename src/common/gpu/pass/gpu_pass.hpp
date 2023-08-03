#pragma once

#include "gpu/gpu_render_target.hpp"
#include "gpu/render_bucket.hpp"


class gpuPass {
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
protected:
    int framebuffer_id = -1;

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
public:
    virtual ~gpuPass() {}

    virtual void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id) = 0;
};