#pragma once


#include <nlohmann/json.hpp>
#include "gpu/gpu_texture_2d.hpp"


bool readGpuTexture2dJson(const nlohmann::json& json, gpuTexture2d* texture);
bool writeGpuTexture2dJson(nlohmann::json& json, gpuTexture2d* texture);

bool readGpuTexture2dBytes(const void* data, size_t sz, gpuTexture2d* texture);
bool writeGpuTexture2dBytes(std::vector<unsigned char>& out, gpuTexture2d* texture);