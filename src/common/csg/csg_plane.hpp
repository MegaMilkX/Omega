#pragma once

#include "math/gfxm.hpp"
#include "nlohmann/json.hpp"


struct csgLine {
    gfxm::vec3 N; // direction
    gfxm::vec3 P; // starting point
};
struct csgMaterial;
struct csgPlane {
    gfxm::vec3 N;
    float D;
    csgMaterial* material = 0;
    gfxm::vec2 uv_scale = gfxm::vec2(1.f, 1.f);
    gfxm::vec2 uv_offset = gfxm::vec2(.0f, .0f);

    void serializeJson(nlohmann::json& json);
    bool deserializeJson(const nlohmann::json& json);
};
