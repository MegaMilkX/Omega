#pragma once

#include "vfx_material.auto.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_uniform_buffer.hpp"
#include "gpu/gpu_texture_2d.hpp"


struct VFXMaterial_ShaderKey : public ShaderKey {
    bool depth_test = true;
    bool soft_clipping = false;

    uint64_t hash() const override {
        uint64_t k = 0;
        k |= uint64_t(depth_test) << 0;
        k |= uint64_t(soft_clipping && depth_test) << 1;
        return k;
    }
    std::string makePrefix() const override {
        std::string out;
        if(depth_test) out += "#define ENABLE_DEPTH_TEST\n";
        if(soft_clipping && depth_test) out += "#define ENABLE_SOFT_CLIPPING\n";
        return out;
    }
};

[[cppi_class]];
class VFXMaterial : public gpuMaterial {
    VFXMaterial_ShaderKey key;

    gpuUniformBufferDesc ubdesc;
    std::unique_ptr<gpuUniformBuffer> ubuf;

    ResourceRef<gpuTexture2d> albedo_map;
    gfxm::vec4 rgba = gfxm::vec4(1, 1, 1, 1);
    float depth_bias = .0f;
    float soft_clip_dist = .5f;

    void updateShaderFlags();
public:
    TYPE_ENABLE();

    VFXMaterial();

    void applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) override;
    void onTick(float dt) override;

    void makeSnapshot(rtti::PropSnapshot&) override;
    void applySnapshot(rtti::PropSnapshot&) override;
};