#pragma once

#include <stdio.h>
#include <stdint.h>
#include "gpu/gpu_material.hpp"
#include "resource_manager/resource_ref.hpp"


bool hl2LoadMaterialFromMemory(const void* data, uint64_t size, ResourceRef<gpuMaterial>& material, const char* path_hint);
bool hl2LoadMaterial(const char* path, ResourceRef<gpuMaterial>& material);

void hl2StoreMaterial(const char* path, ResourceRef<gpuMaterial>& material);
