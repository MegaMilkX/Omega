#pragma once

#include "gpu/gpu_render_target.hpp"
#include "gpu/render_bucket.hpp"


class gpuPass {
protected:
    int framebuffer_id = -1;
public:
    virtual ~gpuPass() {}

    virtual void onDraw(gpuRenderTarget* target, gpuRenderBucket* bucket, int technique_id) = 0;
};