#pragma once

#include <memory>
#include "gpu/gpu_framebuffer.hpp"
#include "gpu/gpu_render_target.hpp"


class gpuRenderTargetMap {
    friend class gpuPipeline;
    friend class gpuRenderTarget;

    gpuRenderTarget* target = nullptr;
    std::vector<int> lwt_array;
    std::vector<std::unique_ptr<gpuFrameBuffer>> framebuffers;
public:
    ~gpuRenderTargetMap();
    const gpuFrameBuffer* getFrameBuffer(int i) const { return framebuffers[i].get(); }
    const gpuRenderTarget* getTarget() const { return target; }
    gpuTexture2d* getTexture(const char* name, int buffer_idx = RT_BUFFER_LAST_WRITTEN);
    int getLwt(int i) const { return lwt_array[i]; }
    void updateTargetLwts();
    void updateSizes();
};

