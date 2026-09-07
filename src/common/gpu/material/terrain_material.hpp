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
        pass_requirement = GPU_MaterialPassReq(GPU_ShadingStyle::Opaque);

        registerVertexSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.vert"));
        registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/terrain.frag"));
    }

    bool resolvePass(GPU_RenderDomain domain, PassResolution& out) const override;
    void applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) override;

    void makeSnapshot(rtti::PropSnapshot&) const override;
    void applySnapshot(const rtti::PropSnapshot&) override;

    bool fromJson(const nlohmann::json&) override;
};

