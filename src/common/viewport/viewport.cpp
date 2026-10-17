#include "viewport.hpp"

#include "gpu/gpu.hpp"
#include "gpu/renderer.hpp"


EngineRenderView::EngineRenderView(const gfxm::rect& rc, RendererType renderer_type, bool is_offscreen)
    : rc(rc), is_offscreen(is_offscreen)
    , render_bucket(gpuGetPipeline(), 1000)
{
    renderer = gpuGetPipeline()->createRenderer(renderer_type);
    renderer->initView(this);
}

EngineRenderView::~EngineRenderView() {
    if (renderer) {
        delete renderer;
    }
}

