#include "gpu/gpu_render_target.hpp"

#include "gpu/gpu_pipeline.hpp"


gpuRenderTarget::~gpuRenderTarget() {
    assert(pipeline);
    pipeline->notifyRenderTargetDestroyed(this);
}

void gpuRenderTarget::updateDirty() {
    assert(pipeline);
    pipeline->updateDirty();
    // TODO: Handle resize here
}

void gpuRenderTarget::setDefaultOutput(const char* name, RT_OUTPUT output_mode) {
    assert(pipeline);
    int idx = pipeline->getChannelIndex(name);
    assert(idx >= 0);
    default_output_texture = idx;
    default_output_mode = output_mode;
}

// TODO: Handle double buffered
gpuTexture2d* gpuRenderTarget::getTexture(const char* name, int buffer_idx) {
    assert(pipeline);
    int idx = pipeline->getChannelIndex(name);
    assert(idx >= 0);

    if (buffer_idx == RT_BUFFER_LAST_WRITTEN) {
        return layers[idx].textures[layers[idx].lwt].get();
    }

    return layers[idx].textures[buffer_idx].get();
}
// TODO: Handle double buffered
HSHARED<gpuTexture2d> gpuRenderTarget::getTextureSharedHandle(const char* name, int buffer_idx) {
    assert(pipeline);
    int idx = pipeline->getChannelIndex(name);
    assert(idx >= 0);

    if (buffer_idx == RT_BUFFER_LAST_WRITTEN) {
        return layers[idx].textures[layers[idx].lwt];
    }

    return layers[idx].textures[buffer_idx];
}

void gpuRenderTarget::bindFrameBuffer(const char* pass_path) {
    assert(pipeline);
    int idx = pipeline->getFrameBufferIndex(pass_path);
    assert(idx >= 0);
    gpuFrameBufferBind(framebuffers[idx].get());

    glViewport(0, 0, width, height);
    glScissor(0, 0, width, height);
}

void gpuRenderTarget::setSize(int width, int height) {
    if (this->width == width && this->height == height) {
        return;
    }
    this->width = width;
    this->height = height;
    if (getPipeline()) {
        for (int i = 0; i < layers.size(); ++i) {
            auto pipeline_channel = getPipeline()->getChannel(i);
            layers[i].textures[0]->resize(width, height);
            if (pipeline_channel->is_double_buffered) {
                layers[i].textures[1]->resize(width, height);
            }
        }
    }
}

void gpuRenderTarget::setDebugRenderGeometryRange(int begin, int end) {
    dbg_geomRangeBegin = begin;
    dbg_geomRangeEnd = end;
}