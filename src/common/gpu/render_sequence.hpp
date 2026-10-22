#pragma once

#include <vector>
#include "gpu/pass/gpu_pass.hpp"


class gpuRenderSequence {
    void _updateFramebufferSizes();
public:
    std::string dbg_name;

    std::vector<gpuPassInstance> passes;
    std::map<std::string, std::string> overrides;

    void init(
        const std::string& dbg_name,
        std::initializer_list<std::string> pass_list,
        std::initializer_list<std::pair<std::string, std::string>> overrides = {}
    );
    void run(gpuRenderBucket* bucket, gpuRenderTarget* target, const DRAW_PARAMS& params, int fb_idx = -1);
};

