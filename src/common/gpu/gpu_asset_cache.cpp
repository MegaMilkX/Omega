#include "gpu_asset_cache.hpp"

#include "embedded_files/decal.png.h"
#include "embedded_files/nimbusmono_bold.otf.h"


gpuAssetCache::gpuAssetCache() {
	default_decal_shader = resGet<gpuShaderProgram>("shaders/decal.glsl");

    default_decal_texture.reset_acquire();
    ktImage img;
    loadImage(&img, decal_png, sizeof(decal_png));
    default_decal_texture->setData(&img);

    default_decal_material.reset_acquire();
    auto tech = default_decal_material->addTechnique("Decals");
    auto pass = tech->addPass();
    pass->setShader(default_decal_shader);
    pass->depth_write = 0;
    //pass->depth_test = 0;
    //pass->cull_faces = 0;
    pass->blend_mode = GPU_BLEND_MODE::ADD;
    default_decal_material->addPassOutputSampler("Normal");
    default_decal_material->addPassOutputSampler("Depth");
    default_decal_material->addSampler("tex", default_decal_texture);
    default_decal_material->compile();


    default_typeface.reset(new Typeface);
    typefaceLoad(default_typeface.get(), (void*)nimbusmono_bold_otf, sizeof(nimbusmono_bold_otf));

    default_font.reset(new Font);
    default_font->init(default_typeface, 16, 72);
}
gpuAssetCache::~gpuAssetCache() {

}

RHSHARED<gpuMaterial> gpuAssetCache::getDefaultDecalMaterial() {
    return default_decal_material;
}
std::shared_ptr<Typeface> gpuAssetCache::getDefaultTypeface() {
    return default_typeface;
}
std::shared_ptr<Font> gpuAssetCache::getDefaultFont() {
    return default_font;
}
