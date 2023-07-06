#include "gpu/gpu_render_target.hpp"

#include "gpu/gpu_pipeline.hpp"



void gpuRenderTarget::setDefaultOutput(const char* name) {
    assert(pipeline);
    int idx = pipeline->getTargetLayerIndex(name);
    assert(idx >= 0);
    default_output_texture = idx;
}

gpuTexture2d* gpuRenderTarget::getTexture(const char* name) {
    assert(pipeline);
    int idx = pipeline->getTargetLayerIndex(name);
    assert(idx >= 0);
    return textures[idx].get();
}

void gpuRenderTarget::bindFrameBuffer(const char* technique, int pass) {
    assert(pipeline);
    int idx = pipeline->getFrameBufferIndex(technique, pass);
    assert(idx >= 0);
    gpuFrameBufferBind(framebuffers[idx].get());

    glViewport(0, 0, width, height);
    glScissor(0, 0, width, height);
}