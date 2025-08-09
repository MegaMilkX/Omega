#pragma once

#include <stdio.h>
#include "gpu/gpu_texture_2d.hpp"


bool hl2LoadTextureFromFile(FILE* f, RHSHARED<gpuTexture2d>& texture);
bool hl2LoadTexture(const char* path, RHSHARED<gpuTexture2d>& texture);

void hl2StoreTexture(const char* path, const RHSHARED<gpuTexture2d>& texture);