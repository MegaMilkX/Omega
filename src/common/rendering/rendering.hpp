#pragma once

#include "gpu/gpu_texture_2d.hpp"

struct renViewportData {
    gpuTexture2d albedo;
    gpuTexture2d depth;
};