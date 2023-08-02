#include "rw_gpu_shader_program.hpp"


bool readGpuShaderProgramJson(const nlohmann::json& json, gpuShaderProgram* prog) {
    return true;
}
bool writeGpuShaderProgramJson(nlohmann::json& json, gpuShaderProgram* prog) {
    return true;
}

bool readGpuShaderProgramBytes(const void* data, size_t sz, gpuShaderProgram* prog) {
    return true;
}
bool writeGpuShaderProgramBytes(std::vector<unsigned char>& out, gpuShaderProgram* prog) {
    return true;
}
