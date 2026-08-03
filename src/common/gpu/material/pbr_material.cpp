#include "pbr_material.hpp"

#include "gpu/gpu.hpp"


void applySampler(gpuShaderProgram* prog, ShaderSamplerSet& out, const char* name, const ResourceRef<gpuTexture2d>& ref) {
    if (!ref) {
        return;
    }

    int slot = prog->getDefaultSamplerSlot(name);
    if (slot < 0) {
        return;
    }

    ShaderSamplerSet::Sampler sampler;
    sampler.source = SHADER_SAMPLER_SOURCE_GPU;
    sampler.type = SHADER_SAMPLER_TEXTURE2D;
    sampler.slot = slot;
    sampler.texture_id = ref->getId();
    out.add(sampler);
}

void PBRMaterial::applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) {
    applySampler(prog, out, "texAlbedo", albedo_map ? albedo_map : getDefaultTexture("WHITE"));
    applySampler(prog, out, "texNormal", normal_map ? normal_map : getDefaultTexture("texNormal"));
    applySampler(prog, out, "texRoughness", roughness_map ? roughness_map : getDefaultTexture("WHITE"));
    applySampler(prog, out, "texMetallic", metallic_map ? metallic_map : getDefaultTexture("WHITE"));
    applySampler(prog, out, "texEmission", emission_map ? emission_map : getDefaultTexture("texEmission"));
    applySampler(prog, out, "texAmbientOcclusion", ao_map ? ao_map : getDefaultTexture("texAmbientOcclusion"));
    applySampler(prog, out, "texDisplacement", displacement_map ? displacement_map : getDefaultTexture("BLACK"));
}

void PBRMaterial::makeSnapshot(rtti::PropSnapshot& snap) {
    snap.type_ = get_type();
    snap.add("rgba", rtti::varying::make<gfxm::vec4>(gfxm::vec4(1, 1, 1, 1)), "lol");

    snap.add("albedo_map", rtti::varying::make(albedo_map), "lol");
    snap.add("normal_map", rtti::varying::make(normal_map), "lol");
    snap.add("roughness_map", rtti::varying::make(roughness_map), "lol");
    snap.add("roughness", rtti::varying::make<float>(1.f), "lol");
    snap.add("metallic_map", rtti::varying::make(metallic_map), "lol");
    snap.add("metallic", rtti::varying::make<float>(.0f), "lol");
    snap.add("ambient_occlusion_map", rtti::varying::make(ao_map), "lol");
    snap.add("emission_map", rtti::varying::make(emission_map), "lol");
    snap.add("emission", rtti::varying::make(gfxm::vec3(0, 0, 0)), "lol");
    snap.add("use_parallax", rtti::varying::make(use_parallax), "lol");
    snap.add("displacement_map", rtti::varying::make(displacement_map), "lol");

    gpuMaterial::makeSnapshot(snap);
}

void PBRMaterial::applySnapshot(rtti::PropSnapshot& snap) {
    if (auto col = snap.get<gfxm::vec4>("rgba")) {
        ubuf->setVec4(ubuf->getDesc()->getUniform("albedo_color"), *col);
    }

    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("albedo_map")) {
        albedo_map = *map;
    }
    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("normal_map")) {
        normal_map = *map;
    }
    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("roughness_map")) {
        roughness_map = *map;
    }
    if (auto col = snap.get<float>("roughness")) {
        ubuf->setFloat(ubuf->getDesc()->getUniform("roughness"), *col);
    }
    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("metallic_map")) {
        metallic_map = *map;
    }
    if (auto col = snap.get<float>("metallic")) {
        ubuf->setFloat(ubuf->getDesc()->getUniform("metallic"), *col);
    }
    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("ambient_occlusion_map")) {
        ao_map = *map;
    }
    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("emission_map")) {
        emission_map = *map;
    }
    if (auto col = snap.get<gfxm::vec3>("emission")) {
        ubuf->setVec3(ubuf->getDesc()->getUniform("emission_color"), *col);
    }
    if (auto val = snap.get<bool>("use_parallax")) {
        use_parallax = *val;
    }
    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("displacement_map")) {
        displacement_map = *map;
    }

    updateShaderFlags();

    gpuMaterial::applySnapshot(snap);
}

void PBRMaterial::toJson(nlohmann::json& json) const {
    json = nlohmann::json::object();
    json["@type"] = rtti::type_get<PBRMaterial>().get_name();

    rtti::type_write_json(
        json["rgba"],
        ubuf->getValue<gfxm::vec4>(ubuf->getDesc()->getUniform("albedo_color"))
    );
    rtti::type_write_json(
        json["emission"],
        ubuf->getValue<gfxm::vec3>(ubuf->getDesc()->getUniform("emission_color"))
    );
    rtti::type_write_json(
        json["roughness"],
        ubuf->getValue<float>(ubuf->getDesc()->getUniform("roughness"))
    );
    rtti::type_write_json(
        json["metallic"],
        ubuf->getValue<float>(ubuf->getDesc()->getUniform("metallic"))
    );

    json["use_parallax"] = use_parallax;

    type_write_json(json["albedo_map"], albedo_map);
    type_write_json(json["normal_map"], normal_map);
    type_write_json(json["roughness_map"], roughness_map);
    type_write_json(json["metallic_map"], metallic_map);
    type_write_json(json["ao_map"], ao_map);
    type_write_json(json["emission_map"], emission_map);
    type_write_json(json["displacement_map"], displacement_map);

    json["transparent"] = getTransparent();
    json["depth_write"] = getDepthWrite();
    json["depth_test"] = getDepthTest();
    json["stencil_test"] = getStencilTest();
    json["cull_faces"] = getBackfaceCulling();
    json["blend_mode"] = (int)getBlendingMode();
    json["sort_bias"] = getSortBias();
    /*
    nlohmann::json& jsamplers = json["samplers"];
    for (int i = 0; i < samplerCount(); ++i) {
        std::string sampler_name = getSamplerName(i);
        const ResourceRef<gpuTexture2d>& tex_ref = getSampler(i);

        if (!tex_ref) {
            jsamplers[sampler_name] = nullptr;
            continue;
        }

        std::string res_id = tex_ref.getResourceId();
        if (!res_id.empty()) {
            jsamplers[sampler_name] = res_id;
            continue;
        }

        ktImage img;
        tex_ref->getData(&img);
        std::vector<uint8_t> bytes;
        writeImagePng(bytes, &img);
        std::string b64;
        base64_encode(bytes.data(), bytes.size(), b64);

        nlohmann::json& jsampler_object = jsamplers[sampler_name];
        jsampler_object = nlohmann::json::object();
        jsampler_object["data"] = b64;
    }*/
}
bool PBRMaterial::fromJson(const nlohmann::json& json) {
    gfxm::vec4 albedo_color(1, 1, 1, 1);
    rtti::type_read_json<gfxm::vec4>(json.value("rgba", nlohmann::json()), albedo_color);
    ubuf->setVec4Staging(ubuf->getDesc()->getUniform("albedo_color"), albedo_color);

    gfxm::vec3 emission_color(0, 0, 0);
    rtti::type_read_json<gfxm::vec3>(json.value("emission", nlohmann::json()), emission_color);
    ubuf->setVec3Staging(ubuf->getDesc()->getUniform("emission_color"), emission_color);

    float roughness = 1.f;
    rtti::type_read_json<float>(json.value("roughness", nlohmann::json()), roughness);
    ubuf->setFloatStaging(ubuf->getDesc()->getUniform("roughness"), roughness);

    float metallic = .0f;
    rtti::type_read_json<float>(json.value("metallic", nlohmann::json()), metallic);
    ubuf->setFloatStaging(ubuf->getDesc()->getUniform("metallic"), metallic);

    ubuf->upload();

    use_parallax = json.value("use_parallax", false);

    type_read_json(json.value("albedo_map", nlohmann::json()), albedo_map);
    type_read_json(json.value("normal_map", nlohmann::json()), normal_map);
    type_read_json(json.value("roughness_map", nlohmann::json()), roughness_map);
    type_read_json(json.value("metallic_map", nlohmann::json()), metallic_map);
    type_read_json(json.value("ao_map", nlohmann::json()), ao_map);
    type_read_json(json.value("emission_map", nlohmann::json()), emission_map);
    type_read_json(json.value("displacement_map", nlohmann::json()), displacement_map);

    setTransparent(json.value("transparent", false));
    setDepthTest(json.value("depth_test", true));
    setDepthWrite(json.value("depth_write", true));
    setStencilTest(json.value("stencil_test", false));
    setBackfaceCulling(json.value("cull_faces", true));
    setBlendingMode((GPU_BLEND_MODE)json.value("blend_mode", 0));
    setSortBias(json.value("sort_bias", 0));
    
    updateShaderFlags();
    /*
    nlohmann::json jsamplers = json.value("samplers", nlohmann::json::object());
    if(jsamplers.is_object()) {
        for(auto it = jsamplers.begin(); it != jsamplers.end(); ++it) {
            std::string name = it.key();
            ResourceRef<gpuTexture2d> htex;
            if(rtti::type_get<ResourceRef<gpuTexture2d>>().deserialize_json(it.value(), &htex)) {
                if(htex) {
                    addSampler(name.c_str(), htex);
                }
            }
        }
    } else if(!jsamplers.is_null()) {
        LOG_ERR("samplers must be an object");
        assert(false);
    }*/

    return true;
}

void PBRMaterial::write(byte_writer& out) const {
    nlohmann::json json;
    toJson(json);
    std::string str = json.dump(2);
    out.write(str.data(), str.size());
}
