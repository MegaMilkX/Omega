#pragma once

#include <nlohmann/json.hpp>
#include "common/render/gpu_material.hpp"


bool readGpuMaterialJson(nlohmann::json& json, gpuMaterial* material);
bool writeGpuMaterialJson(nlohmann::json& json, gpuMaterial* material);
