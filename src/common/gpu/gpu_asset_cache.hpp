#pragma once

#include "handle/hshared.hpp"
#include "gpu/gpu_material.hpp"
#include "gpu/gpu_shader_program.hpp"
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_cube_map.hpp"
#include "typeface/typeface.hpp"
#include "typeface/font.hpp"


class gpuAssetCache {
    RHSHARED<gpuTexture2d> default_decal_texture;
    RHSHARED<gpuShaderProgram> default_decal_shader;
    RHSHARED<gpuMaterial> default_decal_material;

    RHSHARED<Typeface> default_typeface;
    RHSHARED<Font> default_font;
public:
    gpuAssetCache();
    ~gpuAssetCache();

    RHSHARED<gpuMaterial> getDefaultDecalMaterial();
    RHSHARED<Typeface> getDefaultTypeface();
    RHSHARED<Font> getDefaultFont();
    
};