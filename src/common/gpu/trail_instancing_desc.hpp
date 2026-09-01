#pragma once

#include "gpu_instancing_desc.hpp"
#include "gpu/shader_set.hpp"
#include "gpu/texture/buffer_texture.hpp"


class gpuTrailInstancingDesc : public gpuInstancingDesc {
    ResourceRef<gpuShaderSet> shader_set;
    gpuBuffer gpu_buffer;
public:
    struct Instance {
        float length_distance;
        float reserved_0;
        float reserved_1;
        float reserved_2;
    };
    HSHARED<gpuBufferTexture> lut_;

    gpuTrailInstancingDesc();

    void setArray(Instance* instances, int count);

    void apply(GPU_INTERMEDIATE_PASS_DESC& pass) const override;
};


