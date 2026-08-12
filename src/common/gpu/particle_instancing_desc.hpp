#pragma once

#include "gpu_instancing_desc.hpp"
#include "gpu/shader_set.hpp"


class gpuParticleInstancingDesc : public gpuInstancingDesc {
    ResourceRef<gpuShaderSet> shader_set;
    gpuBuffer gpu_buffer;
public:
    struct Instance {
        gfxm::vec4 pos;
        gfxm::vec4 scale;
        gfxm::vec4 rgba;
        gfxm::vec4 sprite_data;
        gfxm::vec4 uv = gfxm::vec4(0, 0, 1, 1); // xy - offset, zw - scale
        gfxm::quat quat;
    };

    gpuParticleInstancingDesc();

    void setArray(Instance* instances, int count);

    void apply(GPU_INTERMEDIATE_PASS_DESC& ctx) const override;
};


