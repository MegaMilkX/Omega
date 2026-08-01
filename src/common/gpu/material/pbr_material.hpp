#pragma once

#include "pbr_material.auto.hpp"
#include "gpu/gpu_material.hpp"


[[cppi_class]];
class PBRMaterial : public gpuMaterial {
public:
    TYPE_ENABLE();

    PBRMaterial() {
        registerVertexSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.vert"));
        registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.frag"));
    }

    bool fromJson(const nlohmann::json&) override;
};

