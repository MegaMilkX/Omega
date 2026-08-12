#include "water_material.hpp"

void WaterMaterial::applySamplers(gpuShaderProgram* prog, ShaderSamplerSet& out) {
    out.addTexture2d(prog, "texNormal", normal_map ? normal_map : getDefaultTexture("texNormal"));
}

bool WaterMaterial::fromJson(const nlohmann::json& json) {
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