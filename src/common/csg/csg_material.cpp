#include "csg_material.hpp"

#include "reflection/serialization.hpp"


void csgMaterial::serializeJson(nlohmann::json& json) {
    rtti::type_write_json(json["name"], name);
    rtti::type_write_json(json["render_material"], gpu_material);
}
bool csgMaterial::deserializeJson(const nlohmann::json& json) {
    rtti::type_read_json(json["name"], name);
    rtti::type_read_json(json["render_material"], gpu_material);
    return true;
}
