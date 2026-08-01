#pragma once

#include "water_material.auto.hpp"
#include "gpu/gpu_material.hpp"


[[cppi_class]];
class WaterMaterial : public gpuMaterial {
public:
    TYPE_ENABLE();

    WaterMaterial() {
        registerVertexSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.vert"));
        registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/placeholder.frag"));
    }

    bool fromJson(const nlohmann::json&) override;
};

