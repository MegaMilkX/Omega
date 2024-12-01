#include "csg_plane.hpp"

#include "reflection/reflection.hpp"


void csgPlane::serializeJson(nlohmann::json& json) {
    type_write_json(json["N"], N);
    type_write_json(json["D"], D);
    type_write_json(json["uv_scale"], uv_scale);
    type_write_json(json["uv_offset"], uv_offset);
}
bool csgPlane::deserializeJson(const nlohmann::json& json) {
    type_read_json(json["N"], N);
    type_read_json(json["D"], D);
    type_read_json(json["uv_scale"], uv_scale);
    type_read_json(json["uv_offset"], uv_offset);
    return true;
}
