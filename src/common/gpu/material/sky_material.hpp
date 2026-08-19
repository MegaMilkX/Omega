#pragma once

#include "sky_material.auto.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_cube_map.hpp"


[[cppi_class]];
class SkyMaterial : public gpuMaterial {
    ResourceRef<gpuCubeMap> sky_map;
public:
    TYPE_ENABLE();

    SkyMaterial();

    void applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) override;

    void makeSnapshot(rtti::PropSnapshot&) override;
    void applySnapshot(rtti::PropSnapshot&) override;
};

