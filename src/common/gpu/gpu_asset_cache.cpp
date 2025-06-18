#include "gpu_asset_cache.hpp"

#include "embedded_files/decal.png.h"
#include "embedded_files/nimbusmono_bold.otf.h"


gpuAssetCache::gpuAssetCache() {
    default_typeface.reset(new Typeface);
    typefaceLoad(default_typeface.get(), (void*)nimbusmono_bold_otf, sizeof(nimbusmono_bold_otf));

    default_font.reset(new Font);
    default_font->init(default_typeface, 16, 72);
}
gpuAssetCache::~gpuAssetCache() {

}

std::shared_ptr<Typeface> gpuAssetCache::getDefaultTypeface() {
    return default_typeface;
}
std::shared_ptr<Font> gpuAssetCache::getDefaultFont() {
    return default_font;
}
