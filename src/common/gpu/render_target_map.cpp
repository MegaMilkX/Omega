#include "render_target_map.hpp"

#include "gpu/gpu.hpp"

gpuTexture2d* gpuRenderTargetMap::getTexture(const char* name, int buffer_idx) {
    assert(gpuGetPipeline());
    int idx = gpuGetPipeline()->getChannelIndex(name);
    assert(idx >= 0);

    if (buffer_idx == RT_BUFFER_LAST_WRITTEN) {
        return target->layers[idx].textures[lwt_array[idx]].get();
    }

    return target->layers[idx].textures[buffer_idx].get();
}

void gpuRenderTargetMap::updateTargetLwts() {
    for (int i = 0; i < lwt_array.size(); ++i) {
        target->layers[i].lwt = lwt_array[i];
    }
}

