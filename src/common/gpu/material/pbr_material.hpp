#pragma once

#include "pbr_material.auto.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_uniform_buffer.hpp"
#include "gpu/gpu_texture_2d.hpp"


[[cppi_class]];
class PBRMaterial : public gpuMaterial {
    gpuUniformBufferDesc ubdesc;
    std::unique_ptr<gpuUniformBuffer> ubuf;

    ResourceRef<gpuTexture2d> albedo_map;
    ResourceRef<gpuTexture2d> normal_map;
    ResourceRef<gpuTexture2d> roughness_map;
    ResourceRef<gpuTexture2d> metallic_map;
    ResourceRef<gpuTexture2d> ao_map;
    ResourceRef<gpuTexture2d> emission_map;
    ResourceRef<gpuTexture2d> displacement_map;

    bool use_parallax = false;

    void updateShaderFlags() {
        uint32_t flags = 0;
        flags |= use_parallax ? 0x01 : 0;
        setShaderFlags(flags);
    }
public:
    TYPE_ENABLE();

    PBRMaterial()
    : ubdesc("ubMaterial") {
        registerVertexSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.vert"));
        registerFragmentSet(loadResource<gpuShaderSet>("core/shaders/modular/basic.frag"));

        ubdesc.define("albedo_color", UNIFORM_TYPE::UNIFORM_VEC4);
        ubdesc.define("emission_color", UNIFORM_TYPE::UNIFORM_VEC3);
        ubdesc.define("roughness", UNIFORM_TYPE::UNIFORM_FLOAT);
        ubdesc.define("metallic", UNIFORM_TYPE::UNIFORM_FLOAT);
        ubdesc.compile();

        ubuf.reset(new gpuUniformBuffer(&ubdesc));
        ubuf->setVec4(ubuf->getDesc()->getUniform("albedo_color"), gfxm::vec4(1, 1, 1, 1));

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

    void makeSnapshot(rtti::PropSnapshot&) override;
    void applySnapshot(rtti::PropSnapshot&) override;

    void toJson(nlohmann::json&) const override;
    bool fromJson(const nlohmann::json&) override;
    void write(byte_writer& out) const override;
};

