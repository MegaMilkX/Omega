#pragma once

#include "math/gfxm.hpp"
#include "gpu/gpu_render_target.hpp"
#include "gpu/render_bucket.hpp"


struct scnRenderSceneView {
    gfxm::mat4 projection;
    gfxm::mat4 view;
    gpuRenderTarget* render_target;
    gpuRenderBucket render_bucket;
};
