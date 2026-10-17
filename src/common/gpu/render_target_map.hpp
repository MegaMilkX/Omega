#pragma once

#include <memory>
#include "gpu/gpu_framebuffer.hpp"
#include "gpu/gpu_render_target.hpp"


class gpuRenderTargetMap {
    friend class gpuPipeline;
    friend class gpuRenderTarget;

    gpuRenderTarget* target = nullptr;
    std::vector<int> lwt_array;
public:
    ~gpuRenderTargetMap();
    const gpuRenderTarget* getTarget() const { return target; }
    gpuTexture2d* getTexture(const char* name, int buffer_idx = RT_BUFFER_LAST_WRITTEN);
    void updateTargetLwts();
};

