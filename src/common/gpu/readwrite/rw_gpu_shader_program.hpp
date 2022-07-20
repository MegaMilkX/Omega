#pragma once

#include <nlohmann/json.hpp>
#include "gpu/gpu_shader_program.hpp"


bool readGpuShaderProgramJson(nlohmann::json& json, gpuShaderProgram* prog);
bool writeGpuShaderProgramJson(nlohmann::json& json, gpuShaderProgram* prog);

bool readGpuShaderProgramBytes(const void* data, size_t sz, gpuShaderProgram* prog);
bool writeGpuShaderProgramBytes(std::vector<unsigned char>& out, gpuShaderProgram* prog);