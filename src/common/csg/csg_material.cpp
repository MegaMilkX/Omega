#include "csg_material.hpp"


void csgMaterial::serializeJson(nlohmann::json& json) {
    type_write_json(json["name"], name);
    type_write_json(json["render_material"], gpu_material);
}
bool csgMaterial::deserializeJson(const nlohmann::json& json) {
    type_read_json(json["name"], name);
    type_read_json(json["render_material"], gpu_material);
    return true;
}
