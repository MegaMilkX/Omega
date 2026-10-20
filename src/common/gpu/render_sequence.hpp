#pragma once

#include <vector>
#include "gpu/pass/gpu_pass.hpp"


class gpuRenderSequence {
    void _updateFramebufferSizes();
public:
    std::string dbg_name;

    struct ChannelDesc {
        std::string name;
        uint32_t pipe_channel_index = -1;
    };
    std::vector<gpuPassInstance> passes;
    std::vector<ChannelDesc> channels;
    std::vector<int> lwt_array;

    void init(const std::string& dbg_name, std::initializer_list<std::string> pass_list);
    void run(gpuRenderBucket* bucket, gpuRenderTarget* target, const DRAW_PARAMS& params, int fb_idx = -1);
};

