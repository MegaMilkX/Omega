#pragma once

#include "gpu_instancing_desc.hpp"
#include "gpu/shader_set.hpp"


class gpuDefaultInstancingDesc : public gpuInstancingDesc {
    ResourceRef<gpuShaderSet> shader_set;
    gpuBuffer gpu_buffer;
public:
    struct Instance {
        gfxm::vec4 pos; // w for scale
        gfxm::quat rot;
    };

    gpuDefaultInstancingDesc();

    void setArray(Instance* instances, int count);

    void apply(GPU_INTERMEDIATE_RENDERABLE_CONTEXT& ctx) const override;
};


