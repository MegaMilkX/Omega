#pragma once

#include <nlohmann/json.hpp>
#include "gpu/gpu_mesh.hpp"


bool readGpuMeshJson(const nlohmann::json& json, gpuMesh* mesh);
bool writeGpuMeshJson(nlohmann::json& json, gpuMesh* mesh);

bool readGpuMeshBytes(const void* data, size_t sz, gpuMesh* mesh);
bool writeGpuMeshBytes(std::vector<unsigned char>& out, gpuMesh* mesh);
