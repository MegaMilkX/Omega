#pragma once

#include <stdio.h>
#include <stdint.h>
#include "gpu/gpu_material.hpp"


bool hl2LoadMaterialFromMemory(const void* data, uint64_t size, RHSHARED<gpuMaterial>& material, const char* path_hint);
bool hl2LoadMaterial(const char* path, RHSHARED<gpuMaterial>& material);

void hl2StoreMaterial(const char* path, RHSHARED<gpuMaterial>& material);
