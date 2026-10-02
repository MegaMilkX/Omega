#pragma once

#include "vfx_material.auto.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_uniform_buffer.hpp"
#include "gpu/texture/texture2d.hpp"


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
    ResourceRef<gpuTexture2d> texture2;
    gfxm::vec4 rgba = gfxm::vec4(1, 1, 1, 1);
    float depth_bias = .0f;
    float soft_clip_dist = .5f;

    GPU_UVScrollMode uv_scroll_mode = GPU_UVScrollMode::None;
    gfxm::vec2 uv_velocity;
    float uv_step_interval = 0.0f;
    int uv_flipbook_cols = 1;
    int uv_flipbook_rows = 1;
    float uv_flipbook_fps = 15.0f;

    GPU_UVScrollMode uv2_scroll_mode = GPU_UVScrollMode::None;
    gfxm::vec2 uv2_velocity;
    float uv2_step_interval = 0.0f;
    int uv2_flipbook_cols = 1;
    int uv2_flipbook_rows = 1;
    float uv2_flipbook_fps = 15.0f;

    float time = .0f;

    void updateShaderFlags();
public:
    TYPE_ENABLE();

    VFXMaterial();

    void setBaseTexture(const ResourceRef<gpuTexture2d>& map) { albedo_map = map; touchVersion(); }
    void setTexture2(const ResourceRef<gpuTexture2d>& map) { texture2 = map; touchVersion(); }

    void setUVScrollMode(GPU_UVScrollMode mode) { uv_scroll_mode = mode; updateShaderFlags(); touchVersion(); }
    void setUVScrollVelocity(const gfxm::vec2& v) { uv_velocity = v; touchVersion(); }
    void setUV2ScrollMode(GPU_UVScrollMode mode) { uv2_scroll_mode = mode; updateShaderFlags(); touchVersion(); }
    void setUV2ScrollVelocity(const gfxm::vec2& v) { uv2_velocity = v; touchVersion(); }

    void setDepthTest_(bool v) {
        // TODO: remove depth test param from base gpuMaterial?
        setDepthTest(v);
        key.depth_test = v;
        touchVersion();
    }

    bool resolvePass(GPU_RenderDomain domain, PassResolution& out) const override;
    void applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) override;
    void onTick(float dt) override;

    void makeSnapshot(rtti::PropSnapshot&) const override;
    void applySnapshot(const rtti::PropSnapshot&) override;
};