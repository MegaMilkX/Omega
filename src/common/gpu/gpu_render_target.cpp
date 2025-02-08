#include "gpu/gpu_render_target.hpp"

#include "gpu/gpu_pipeline.hpp"



void gpuRenderTarget::setDefaultOutput(const char* name) {
    assert(pipeline);
    int idx = pipeline->getTargetLayerIndex(name);
    assert(idx >= 0);
    default_output_texture = idx;
}

// TODO: Handle double buffered
gpuTexture2d* gpuRenderTarget::getTexture(const char* name) {
    assert(pipeline);
    int idx = pipeline->getTargetLayerIndex(name);
    assert(idx >= 0);
    return layers[idx].texture_a.get();
}
// TODO: Handle double buffered
HSHARED<gpuTexture2d> gpuRenderTarget::getTextureSharedHandle(const char* name) {
    assert(pipeline);
    int idx = pipeline->getTargetLayerIndex(name);
    assert(idx >= 0);
    return layers[idx].texture_a;
}

void gpuRenderTarget::bindFrameBuffer(const char* technique, int pass) {
    assert(pipeline);
    int idx = pipeline->getFrameBufferIndex(technique, pass);
    assert(idx >= 0);
    gpuFrameBufferBind(framebuffers[idx].get());

    glViewport(0, 0, width, height);
    glScissor(0, 0, width, height);
}


void gpuRenderTarget::setDebugRenderGeometryRange(int begin, int end) {
    dbg_geomRangeBegin = begin;
    dbg_geomRangeEnd = end;
}