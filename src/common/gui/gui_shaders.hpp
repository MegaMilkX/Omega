#pragma once

#include "gpu/gpu_pipeline.hpp"


void _guiInitShaders();
void _guiCleanupShaders();
gpuShaderProgram* _guiGetShaderTextSelection();
gpuShaderProgram* _guiGetShaderText();
gpuShaderProgram* _guiGetShaderRect();