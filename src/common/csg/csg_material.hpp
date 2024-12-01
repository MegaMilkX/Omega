#pragma once

#include <string>
#include <memory>
#include "nlohmann/json.hpp"
#include "handle/hshared.hpp"
#include "gpu/gpu_material.hpp"


struct csgMaterial {
    int index = -1;
    std::string name;
    std::unique_ptr<gpuTexture2d> texture;
    RHSHARED<gpuMaterial> gpu_material;

    void serializeJson(nlohmann::json& json);
    bool deserializeJson(const nlohmann::json& json);
};
