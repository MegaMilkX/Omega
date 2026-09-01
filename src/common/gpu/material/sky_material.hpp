#pragma once

#include "sky_material.auto.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/texture/cube_texture.hpp"


[[cppi_class]];
class SkyMaterial : public gpuMaterial {
    ResourceRef<gpuCubeTexture> sky_map;
public:
    TYPE_ENABLE();

    SkyMaterial();

    bool resolvePass(GPU_RenderDomain domain, PassResolution& out) const override;
    void applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) override;

    void makeSnapshot(rtti::PropSnapshot&) override;
    void applySnapshot(rtti::PropSnapshot&) override;
};

