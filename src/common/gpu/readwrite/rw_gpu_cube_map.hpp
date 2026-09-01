#pragma once


#include <nlohmann/json.hpp>
#include "gpu/texture/cube_texture.hpp"


bool readGpuCubeMapJson(const nlohmann::json& json, gpuCubeTexture* texture);
bool writeGpuCubeMapJson(nlohmann::json& json, gpuCubeTexture* texture);

bool readGpuCubeMapBytes(const void* data, size_t sz, gpuCubeTexture* texture);
bool writeGpuCubeMapBytes(std::vector<unsigned char>& out, gpuCubeTexture* texture);