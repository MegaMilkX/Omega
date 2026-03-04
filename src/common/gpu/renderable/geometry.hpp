#pragma once

#include "gpu/gpu_renderable.hpp"
#include "gpu/param_block/transform_block.hpp"


class gpuGeoRenderable : public gpuRenderable {
    gpuTransformBlock* transform_block = nullptr;
public:
    gpuGeoRenderable();
    gpuGeoRenderable(gpuMaterial* mat, const gpuMeshDesc* mesh, const gpuInstancingDesc* instancing = 0, const char* dbg_name = "noname");
    ~gpuGeoRenderable();

    void setTransform(const gfxm::mat4& t) {
        transform_block->setTransform(t);
        updateSortHint(t[3]);
    }
};