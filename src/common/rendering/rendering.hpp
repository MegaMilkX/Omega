#pragma once

#include "gpu/texture/texture2d.hpp"

struct renViewportData {
    gpuTexture2d albedo;
    gpuTexture2d depth;
};