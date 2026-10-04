#pragma once

#include <vector>
#include "gpu/gpu_mesh.hpp"
#include "gpu/gpu_renderable.hpp"
#include "gpu/default_instancing_desc.hpp"
#include "gpu/param_block/transform_block.hpp"

#include "m3d/m3d_model.hpp"
#include "resource_manager/resource_ref.hpp"


struct TerrainDecoration {
    ResourceRef<m3dModel> model;
    std::vector<gpuDefaultInstancingDesc::Instance> instances;
    // Preview
    std::vector<gpuRenderable> renderables;
    gpuDefaultInstancingDesc inst_desc;
    std::vector<gpuTransformBlock*> transform_blocks;
};

struct TerrainCell {
    std::vector<float> points;
    float y_min = .0f;
    float y_max = .0f;

    std::vector<TerrainDecoration> decorations;

    // Preview
    gpuMesh mesh;
    gpuRenderable renderable;
    gpuTransformBlock* transform_block = nullptr;

    TerrainCell() = default;
    TerrainCell(const TerrainCell&) = delete;
    TerrainCell(TerrainCell&&) noexcept = default;
    TerrainCell& operator=(const TerrainCell&) = delete;
    TerrainCell& operator=(TerrainCell&&) noexcept = default;
};

