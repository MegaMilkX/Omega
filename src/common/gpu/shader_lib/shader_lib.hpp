#pragma once

#include "handle/hshared.hpp"
#include "gpu/gpu_shader_program.hpp"


struct SHADER_LIB_LOAD_PARAMS {
    // TODO: Resolve paths into ShaderSource* pointers, THEN traverse the tree
    const char* host_vertex_path;
    const char* host_fragment_path;

    const char* vertex_path;
    const char* geometry_path;
    const char* fragment_path;

    uint64_t flags;
};

RHSHARED<gpuShaderProgram> shaderLibLoadProgram(const SHADER_LIB_LOAD_PARAMS& params);

