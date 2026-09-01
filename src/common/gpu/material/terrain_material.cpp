#include "terrain_material.hpp"


bool TerrainMaterial::resolvePass(GPU_RenderDomain domain, PassResolution& out) const {
    switch (domain) {
    case GPU_RenderDomain::Surface:
        out.pass_name = "Default";
        out.blend_mode = getBlendingMode();
        // TODO: out.draw_flags = 
        out.cast_shadows = true;
        return true;
    }
    return false;
}
void TerrainMaterial::applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) {
    out.addTexture2dRef(prog, "texAlbedo", albedo ? albedo : getDefaultTexture("WHITE"));
    out.addTexture2dRef(prog, "texAlbedo2", albedo2 ? albedo2 : getDefaultTexture("WHITE"));
}

void TerrainMaterial::makeSnapshot(rtti::PropSnapshot& snap) {
    snap.type_ = get_type();
    snap.add("albedo_map", rtti::varying::make(albedo), "TerrainMaterial");
    snap.add("albedo_map2", rtti::varying::make(albedo2), "TerrainMaterial");
    gpuMaterial::makeSnapshot(snap);
}
void TerrainMaterial::applySnapshot(rtti::PropSnapshot& snap) {
    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("albedo_map")) {
        albedo = *map;
    }
    if (auto map = snap.get<ResourceRef<gpuTexture2d>>("albedo_map2")) {
        albedo2 = *map;
    }

    gpuMaterial::applySnapshot(snap);
}

bool TerrainMaterial::fromJson(const nlohmann::json& json) {
    setTransparent(json.value("transparent", false));

    setDepthTest(json.value("depth_test", true));
    setDepthWrite(json.value("depth_write", true));
    setStencilTest(json.value("stencil_test", false));
    setBackfaceCulling(json.value("cull_faces", true));
    setBlendingMode((GPU_BLEND_MODE)json.value("blend_mode", 0));
    setSortBias(json.value("sort_bias", 0));

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
    } else {
        LOG_ERR("samplers must be an object");
        assert(false);
    }

    return true;
}

