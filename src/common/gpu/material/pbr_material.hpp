#pragma once

#include "pbr_material.auto.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_uniform_buffer.hpp"
#include "gpu/gpu_texture_2d.hpp"


[[cppi_enum]];
enum class PBRMaterial_ScrollMode {
    None,
    Smooth,
    Step,
    Flipbook
};

struct PBRMaterial_ShaderKey : public ShaderKey {
    bool use_parallax = false;
    GPU_AlphaMode alpha_mode = GPU_AlphaMode::Opaque;

    uint64_t hash() const override {
        uint64_t k = 0;
        k |= uint64_t(use_parallax) << 0;

        static_assert(int(GPU_AlphaMode::COUNT) == 3);
        k |= uint64_t(alpha_mode) << 1;
        // next offset is 4, alpha mode reserves 3 bits even though it only needs 2 right now
        return k;
    }
    std::string makePrefix() const override {
        std::string out;
        if(use_parallax) out += "#define ENABLE_PARALLAX\n";

        static_assert(int(GPU_AlphaMode::COUNT) == 3);
        out += std::format(
            "#define ALPHA_MODE_OPAQUE {}\n"
            "#define ALPHA_MODE_BLEND {}\n"
            "#define ALPHA_MODE_DISCARD {}\n"
            "#define ALPHA_MODE {}\n",
            int(GPU_AlphaMode::Opaque),
            int(GPU_AlphaMode::Blend),
            int(GPU_AlphaMode::Discard),
            int(alpha_mode)
        );
        return out;
    }
};

[[cppi_class]];
class PBRMaterial : public gpuMaterial {
    PBRMaterial_ShaderKey shader_key;

    gpuUniformBufferDesc ubdesc;
    std::unique_ptr<gpuUniformBuffer> ubuf;

    ResourceRef<gpuTexture2d> albedo_map;
    ResourceRef<gpuTexture2d> normal_map;
    ResourceRef<gpuTexture2d> roughness_map;
    ResourceRef<gpuTexture2d> metallic_map;
    ResourceRef<gpuTexture2d> ao_map;
    ResourceRef<gpuTexture2d> emission_map;
    ResourceRef<gpuTexture2d> displacement_map;

    PBRMaterial_ScrollMode uv_scroll_mode = PBRMaterial_ScrollMode::None;
    gfxm::vec2 uv_velocity;        // smooth/step
    float uv_step_interval = 0.0f; // step (seconds)
    int uv_flipbook_cols = 1;
    int uv_flipbook_rows = 1;
    float uv_flipbook_fps = 15.0f;

    float time = .0f;

    void updateShaderFlags() {
        shading_style = getTransparent() ? GPU_ShadingStyle::ForwardTranslucent : GPU_ShadingStyle::Opaque;
        is_animated = uv_scroll_mode != PBRMaterial_ScrollMode::None;
    }
public:
    TYPE_ENABLE();

    PBRMaterial()
    : ubdesc("ubMaterial") {
        registerShaderKey(&shader_key);
        registerVertexSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.vert"));
        registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.frag"));

        ubdesc.define("albedo_color", UNIFORM_TYPE::UNIFORM_VEC4);
        ubdesc.define("emission_color", UNIFORM_TYPE::UNIFORM_VEC3);
        ubdesc.define("roughness", UNIFORM_TYPE::UNIFORM_FLOAT);
        ubdesc.define("metallic", UNIFORM_TYPE::UNIFORM_FLOAT);
        ubdesc.define("discard_threshold", UNIFORM_TYPE::UNIFORM_FLOAT);
        ubdesc.define("uv_scale", UNIFORM_TYPE::UNIFORM_VEC2);
        ubdesc.define("uv_offset", UNIFORM_TYPE::UNIFORM_VEC2);
        ubdesc.compile();

        ubuf.reset(new gpuUniformBuffer(&ubdesc));
        ubuf->setVec4(ubuf->getDesc()->getUniform("albedo_color"), gfxm::vec4(1, 1, 1, 1));
        ubuf->setVec2(ubuf->getDesc()->getUniform("uv_scale"), gfxm::vec2(1, 1));
        ubuf->setFloat(ubuf->getDesc()->getUniform("roughness"), 1.f);
        ubuf->setFloat(ubuf->getDesc()->getUniform("metallic"), .0f);
        ubuf->setFloat(ubuf->getDesc()->getUniform("discard_threshold"), .5f);

        addUniformBuffer(ubuf.get());

        updateShaderFlags();
    }

    void setAlbedoMap(const ResourceRef<gpuTexture2d>& map) { albedo_map = map; }
    void setNormalMap(const ResourceRef<gpuTexture2d>& map) { normal_map = map; }
    void setRoughnessMap(const ResourceRef<gpuTexture2d>& map) { roughness_map = map; }
    void setMetallicMap(const ResourceRef<gpuTexture2d>& map) { metallic_map = map; }
    void setAOMap(const ResourceRef<gpuTexture2d>& map) { ao_map = map; }
    void setEmissionMap(const ResourceRef<gpuTexture2d>& map) { emission_map = map; }
    void setDisplacementMap(const ResourceRef<gpuTexture2d>& map) { displacement_map = map; }

    void applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) override;
    void onTick(float dt) override;

    void makeSnapshot(rtti::PropSnapshot&) override;
    void applySnapshot(rtti::PropSnapshot&) override;

    void toJson(nlohmann::json&) const override;
    bool fromJson(const nlohmann::json&) override;
    void write(byte_writer& out) const override;
};

