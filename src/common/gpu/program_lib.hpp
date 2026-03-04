#pragma once

#include "handle/hshared.hpp"
#include "gpu/gpu_shader_program.hpp"


RHSHARED<gpuShaderProgram> gpuGetProgram(const gpuCompiledShader** shaders, int count);

