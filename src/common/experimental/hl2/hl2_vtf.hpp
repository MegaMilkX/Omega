#pragma once

#include <stdio.h>
#include "gpu/gpu_texture_2d.hpp"


bool hl2LoadTextureFromFile(FILE* f, ResourceRef<gpuTexture2d>& texture);
bool hl2LoadTexture(const char* path, ResourceRef<gpuTexture2d>& texture);

void hl2StoreTexture(const char* path, const ResourceRef<gpuTexture2d>& texture);