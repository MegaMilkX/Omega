#pragma once

#include <nlohmann/json.hpp>
#include "gpu/gpu_material.hpp"


bool readGpuMaterialJson(const nlohmann::json& json, gpuMaterial* material);
bool writeGpuMaterialJson(nlohmann::json& json, gpuMaterial* material);
