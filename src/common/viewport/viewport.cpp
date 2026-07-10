#include "viewport.hpp"

#include "gpu/gpu.hpp"


EngineRenderView::EngineRenderView(const gfxm::rect& rc, gpuRenderer* renderer, bool is_offscreen)
    : rc(rc), renderer(renderer), is_offscreen(is_offscreen)
    , render_bucket(gpuGetPipeline(), 1000)
{}

