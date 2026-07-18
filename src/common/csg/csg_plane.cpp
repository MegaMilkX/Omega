#include "csg_plane.hpp"

#include "reflection/reflection.hpp"
#include "reflection//serialization.hpp"


void csgPlane::serializeJson(nlohmann::json& json) {
    rtti::type_write_json(json["N"], N);
    rtti::type_write_json(json["D"], D);
    rtti::type_write_json(json["uv_scale"], uv_scale);
    rtti::type_write_json(json["uv_offset"], uv_offset);
}
bool csgPlane::deserializeJson(const nlohmann::json& json) {
    rtti::type_read_json(json["N"], N);
    rtti::type_read_json(json["D"], D);
    rtti::type_read_json(json["uv_scale"], uv_scale);
    rtti::type_read_json(json["uv_offset"], uv_offset);
    return true;
}
