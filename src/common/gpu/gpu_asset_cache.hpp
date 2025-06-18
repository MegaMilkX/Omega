#pragma once

#include "handle/hshared.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_cube_map.hpp"
#include "typeface/typeface.hpp"
#include "typeface/font.hpp"


class gpuAssetCache {
    std::shared_ptr<Typeface> default_typeface;
    std::shared_ptr<Font> default_font;
public:
    gpuAssetCache();
    ~gpuAssetCache();

    std::shared_ptr<Typeface> getDefaultTypeface();
    std::shared_ptr<Font> getDefaultFont();
    
};