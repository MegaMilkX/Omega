#include "gpu/gpu_render_target.hpp"

#include "gpu/gpu_pipeline.hpp"
#include "gpu/render_target_map.hpp"


gpuRenderTarget::~gpuRenderTarget() {
    assert(pipeline);
    pipeline->notifyRenderTargetDestroyed(this);
    for (auto& m : maps) {
        m->target = nullptr;
    }
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
    if (idx < 0) {
        return;
    }
    default_output_texture = idx;
    default_output_mode = output_mode;
}

gpuTexture2d* gpuRenderTarget::getTexture(const char* name, int buffer_idx) {
    assert(pipeline);
    int idx = pipeline->getChannelIndex(name);
    assert(idx >= 0);

    if (buffer_idx == RT_BUFFER_LAST_WRITTEN) {
        return layers[idx].textures[layers[idx].lwt].get();
    }

    return layers[idx].textures[buffer_idx].get();
}

void gpuRenderTarget::setSize(int width, int height) {
    if (this->width == width && this->height == height) {
        return;
    }
    this->width = width;
    this->height = height;
    for (int i = 0; i < layers.size(); ++i) {
        auto& layer = layers[i];
        int w = width;
        int h = height;
        if (layer.explicit_width) {
            w = layer.explicit_width;
        }
        if (layer.explicit_height) {
            h = layer.explicit_height;
        }

        layer.textures[0]->resize(w, h);
        if (layer.textures[1]) {
            layer.textures[1]->resize(w, h);
        }
    }

    for (auto& m : maps) {
        m->updateSizes();
    }
}

void gpuRenderTarget::setDebugRenderGeometryRange(int begin, int end) {
    dbg_geomRangeBegin = begin;
    dbg_geomRangeEnd = end;
}