#pragma once

#include "terrain_material.auto.hpp"
#include "gpu/gpu_material.hpp"


[[cppi_class]];
class TerrainMaterial : public gpuMaterial {
    ResourceRef<gpuTexture2d> albedo;
    ResourceRef<gpuTexture2d> albedo2;
public:
    TYPE_ENABLE();

    TerrainMaterial() {
        registerVertexSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.vert"));
        registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/terrain.frag"));
    }

    void applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) override;

    void makeSnapshot(rtti::PropSnapshot&) override;
    void applySnapshot(rtti::PropSnapshot&) override;

    bool fromJson(const nlohmann::json&) override;
};

