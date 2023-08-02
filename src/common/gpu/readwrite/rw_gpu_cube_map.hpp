#pragma once


#include <nlohmann/json.hpp>
#include "gpu/gpu_cube_map.hpp"


bool readGpuCubeMapJson(const nlohmann::json& json, gpuCubeMap* texture);
bool writeGpuCubeMapJson(nlohmann::json& json, gpuCubeMap* texture);

bool readGpuCubeMapBytes(const void* data, size_t sz, gpuCubeMap* texture);
bool writeGpuCubeMapBytes(std::vector<unsigned char>& out, gpuCubeMap* texture);