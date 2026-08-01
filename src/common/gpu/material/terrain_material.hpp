#pragma once

#include "terrain_material.auto.hpp"
#include "gpu/gpu_material.hpp"


[[cppi_class]];
class TerrainMaterial : public gpuMaterial {
public:
    TYPE_ENABLE();

    TerrainMaterial() {
        registerVertexSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.vert"));
        registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/terrain.frag"));
    }

    bool fromJson(const nlohmann::json&) override;
};

