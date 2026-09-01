#pragma once

#include "water_material.auto.hpp"
#include "gpu/gpu_material.hpp"


[[cppi_class]];
class WaterMaterial : public gpuMaterial {
    ResourceRef<gpuTexture2d> normal_map;
public:
    TYPE_ENABLE();

    WaterMaterial() {
        registerVertexSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.vert"));
        registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/water.mat.frag"));

        normal_map = loadResource<gpuTexture2d>("textures/water_coast01_normal");

        pass_requirement = GPU_MaterialPassReq("HL2/Water");
        //shading_style = GPU_ShadingStyle::WATER;
        is_animated = false;
        setBlendingMode(GPU_BLEND_MODE::BLEND);
    }

    void setNormalMap(const ResourceRef<gpuTexture2d>& map);

    bool resolvePass(GPU_RenderDomain domain, PassResolution& out) const override;
    void applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) override;

    bool fromJson(const nlohmann::json&) override;
};

