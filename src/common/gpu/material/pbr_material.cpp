#include "pbr_material.hpp"

#include "gpu/gpu.hpp"


bool PBRMaterial::resolvePass(GPU_RenderDomain domain, PassResolution& out) const {
    switch (domain) {
    case GPU_RenderDomain::Surface:
        if (getTransparent()) {
            out.pass_name = "HL2/Translucent";
            out.blend_mode = getBlendingMode();
            // TODO: out.draw_flags = 
            out.cast_shadows = false;
        } else {
            out.pass_name = "Default";
            out.blend_mode = GPU_BLEND_MODE::OVERWRITE;
            // TODO: out.draw_flags = 
            out.cast_shadows = true;
        }
        return true;
    case GPU_RenderDomain::Decal:
        if (getTransparent()) {
            out.pass_name = "Decals";
            out.blend_mode = getBlendingMode();
            // TODO: out.draw_flags = 
            out.cast_shadows = false;
        } else {
            out.pass_name = "Decals_GBuffer";
            out.blend_mode = getBlendingMode();
            // TODO: out.draw_flags = 
            out.cast_shadows = false;
        }
        return true;
    }
    return false;
}
void PBRMaterial::applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) {
    out.addTexture2dRef(prog, "texAlbedo", albedo_map ? albedo_map : getDefaultTexture("WHITE"));
    out.addTexture2dRef(prog, "texNormal", normal_map ? normal_map : getDefaultTexture("texNormal"));
    out.addTexture2dRef(prog, "texRoughness", roughness_map ? roughness_map : getDefaultTexture("WHITE"));
    out.addTexture2dRef(prog, "texMetallic", metallic_map ? metallic_map : getDefaultTexture("WHITE"));
    out.addTexture2dRef(prog, "texEmission", emission_map ? emission_map : getDefaultTexture("texEmission"));
    out.addTexture2dRef(prog, "texAmbientOcclusion", ao_map ? ao_map : getDefaultTexture("texAmbientOcclusion"));
    out.addTexture2dRef(prog, "texDisplacement", displacement_map ? displacement_map : getDefaultTexture("BLACK"));
}
void PBRMaterial::onTick(float dt) {
    switch (uv_scroll_mode) {
    case GPU_UVScrollMode::Smooth: {
        gfxm::vec2 offs = ubuf->getValue<gfxm::vec2>(ubuf->getDesc()->getUniform("uv_offset"));
        offs += uv_velocity * dt;
        ubuf->setVec2(ubuf->getDesc()->getUniform("uv_offset"), offs);
        break;
    }
    case GPU_UVScrollMode::Step: {
        gfxm::vec2 offs;
        float t = uv_step_interval > 0.0f
            ? std::floor(time / uv_step_interval) * uv_step_interval
            : time;
        offs.x = uv_velocity.x * t;
        offs.y = uv_velocity.y * t;
        ubuf->setVec2(ubuf->getDesc()->getUniform("uv_offset"), offs);
        break;
    }
    case GPU_UVScrollMode::Flipbook: {
        gfxm::vec2 scale;
        gfxm::vec2 offs;

        int total = uv_flipbook_cols * uv_flipbook_rows;
        int frame = static_cast<int>(time * uv_flipbook_fps);
        frame = (frame % total);
        float sx = 1.0f / uv_flipbook_cols;
        float sy = 1.0f / uv_flipbook_rows;
        scale = gfxm::vec2(sx, sy);
        offs = gfxm::vec2((frame % uv_flipbook_cols) * sx, 1.0f - (frame / uv_flipbook_cols) * sy);

        ubuf->setVec2(ubuf->getDesc()->getUniform("uv_scale"), scale);
        ubuf->setVec2(ubuf->getDesc()->getUniform("uv_offset"), offs);
        break;
    }
    }
    time += dt;
}

void PBRMaterial::makeSnapshot(rtti::PropSnapshot& snap) const {
    snap.type_ = get_type();

    gfxm::vec4 rgba = ubuf->getValue<gfxm::vec4>(ubuf->getDesc()->getUniform("albedo_color"));
    snap.add("rgba", rtti::varying::make<gfxm::vec4>(rgba), "lol");

    snap.add("albedo_map", rtti::varying::make(albedo_map), "lol");
    snap.add("normal_map", rtti::varying::make(normal_map), "lol");
    snap.add("roughness_map", rtti::varying::make(roughness_map), "lol");
    float roughness = ubuf->getValue<float>(ubuf->getDesc()->getUniform("roughness"));
    snap.add("roughness", rtti::varying::make<float>(roughness), "lol");
    snap.add("metallic_map", rtti::varying::make(metallic_map), "lol");
    float metallic = ubuf->getValue<float>(ubuf->getDesc()->getUniform("metallic"));
    snap.add("metallic", rtti::varying::make<float>(metallic), "lol");
    snap.add("ambient_occlusion_map", rtti::varying::make(ao_map), "lol");
    snap.add("emission_map", rtti::varying::make(emission_map), "lol");
    gfxm::vec3 emission = ubuf->getValue<gfxm::vec3>(ubuf->getDesc()->getUniform("emission_color"));
    snap.add("emission", rtti::varying::make(emission), "lol");

    snap.add("use_lightmap", rtti::varying::make(shader_key.use_lightmap), "lol");

    snap.add("use_parallax", rtti::varying::make(shader_key.use_parallax), "lol");
    snap.add("displacement_map", rtti::varying::make(displacement_map), "lol");

    snap.add("alpha_mode", rtti::varying::make(shader_key.alpha_mode), "lol");
    float discard_threshold = ubuf->getValue<float>(ubuf->getDesc()->getUniform("discard_threshold"));
    snap.add("discard_threshold", rtti::varying::make<float>(discard_threshold), "lol");

    snap.add("uv_scroll", rtti::varying::make(uv_scroll_mode), "lol");
    snap.add("uv_velocity", rtti::varying::make(uv_velocity), "lol");
    snap.add("uv_interval", rtti::varying::make(uv_step_interval), "lol");
    snap.add("flipbook_columns", rtti::varying::make(uv_flipbook_cols), "lol");
    snap.add("flipbook_rows", rtti::varying::make(uv_flipbook_rows), "lol");
    snap.add("flipbook_fps", rtti::varying::make(uv_flipbook_fps), "lol");

    gpuMaterial::makeSnapshot(snap);
}

void PBRMaterial::applySnapshot(const rtti::PropSnapshot& snap) {
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

    if (auto val = snap.get<bool>("use_lightmap")) {
        shader_key.use_lightmap = *val;
    }

    if (auto val = snap.get<bool>("use_parallax")) {
        shader_key.use_parallax = *val;
    }
    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("displacement_map")) {
        displacement_map = *map;
    }

    if (auto mode = snap.get<GPU_AlphaMode>("alpha_mode")) {
        shader_key.alpha_mode = *mode;
    }
    if (auto val = snap.get<float>("discard_threshold")) {
        ubuf->setFloat(ubuf->getDesc()->getUniform("discard_threshold"), *val);
    }

    if (auto val = snap.get<GPU_UVScrollMode>("uv_scroll")) {
        uv_scroll_mode = *val;
    }
    if (auto val = snap.get<gfxm::vec2>("uv_velocity")) {
        uv_velocity = *val;
    }
    if (auto val = snap.get<float>("uv_interval")) {
        uv_step_interval = *val;
    }
    if (auto val = snap.get<int>("flipbook_columns")) {
        uv_flipbook_cols = *val;
    }
    if (auto val = snap.get<int>("flipbook_rows")) {
        uv_flipbook_rows = *val;
    }
    if (auto val = snap.get<float>("flipbook_fps")) {
        uv_flipbook_fps = *val;
    }

    gpuMaterial::applySnapshot(snap);
    updateShaderFlags();
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

    json["use_parallax"] = shader_key.use_parallax;
    rtti::type_get<GPU_UVScrollMode>().serialize_json(json["uv_scroll"], &uv_scroll_mode);

    rtti::type_get<GPU_AlphaMode>().serialize_json(json["alpha_mode"], &shader_key.alpha_mode);
    rtti::type_write_json(
        json["discard_threshold"],
        ubuf->getValue<float>(ubuf->getDesc()->getUniform("discard_threshold"))
    );

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

    shader_key.use_parallax = json.value("use_parallax", false);
    rtti::type_get<GPU_UVScrollMode>().deserialize_json(json.value("uv_scroll", nlohmann::json()), &uv_scroll_mode);

    type_read_json(json.value("albedo_map", nlohmann::json()), albedo_map);
    type_read_json(json.value("normal_map", nlohmann::json()), normal_map);
    type_read_json(json.value("roughness_map", nlohmann::json()), roughness_map);
    type_read_json(json.value("metallic_map", nlohmann::json()), metallic_map);
    type_read_json(json.value("ao_map", nlohmann::json()), ao_map);
    type_read_json(json.value("emission_map", nlohmann::json()), emission_map);
    type_read_json(json.value("displacement_map", nlohmann::json()), displacement_map);

    rtti::type_get<GPU_AlphaMode>().deserialize_json(json.value("alpha_mode", nlohmann::json()), &shader_key.alpha_mode);
    float discard_threshold = .5f;
    rtti::type_read_json<float>(json.value("discard_threshold", nlohmann::json()), discard_threshold);
    ubuf->setFloatStaging(ubuf->getDesc()->getUniform("discard_threshold"), discard_threshold);

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
    rtti::PropSnapshot snap;
    const_cast<PBRMaterial*>(this)->makeSnapshot(snap);
    snap.toJson(json);
    //toJson(json);
    std::string str = json.dump(2);
    out.write(str.data(), str.size());
}
