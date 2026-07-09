#pragma once

#include <vector>
#include "gpu/pass/gpu_pass.hpp"
#include "gpu/render_target_map.hpp"


class gpuRenderSequence {
public:
    struct ChannelDesc {
        std::string name;
        uint32_t pipe_channel_index = -1;
    };
    std::vector<gpuPassInstance> passes;
    std::vector<ChannelDesc> channels;

    void init(std::initializer_list<std::string> pass_list);
    int  getChannelIdx(const std::string& name);
    void run(gpuRenderBucket* bucket, gpuRenderTargetMap* target_map, const DRAW_PARAMS& params);
};

