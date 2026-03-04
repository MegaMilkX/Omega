#pragma once

#include "gpu/gpu_renderable.hpp"
#include "gpu/param_block/transform_block.hpp"
#include "gpu/param_block/decal_block.hpp"


class gpuDecalRenderable : public gpuRenderable {
    gpuTransformBlock* transform_block = nullptr;
    gpuDecalBlock* decal_block = nullptr;
public:
    gpuDecalRenderable();
    gpuDecalRenderable(gpuMaterial* mat, const gpuInstancingDesc* instancing = 0, const char* dbg_name = "noname");
    ~gpuDecalRenderable();

    void setExtents(const gfxm::vec3& extents) {
        decal_block->setExtents(extents);
    }
    void setColor(const gfxm::vec4& col) {
        decal_block->setColor(col);
    }
    void setTransform(const gfxm::mat4& t) {
        transform_block->setTransform(t);
        updateSortHint(t[3]);
    }

    gpuTransformBlock* getTransformBlock() { return transform_block; }
};