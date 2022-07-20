#pragma once

#include <memory>
#include "typeface/font.hpp"
#include "gpu/glx_texture_2d.hpp"

struct GuiFont {
    Font* font;
    std::unique_ptr<gpuTexture2d> atlas;
    std::unique_ptr<gpuTexture2d> lut;
};

inline void guiFontCreate(GuiFont& gui_font, Font* font) {
    ktImage imgFontAtlas;
    ktImage imgFontLookupTexture;
    font->buildAtlas(&imgFontAtlas, &imgFontLookupTexture);
    gui_font.font = font;
    gui_font.atlas.reset(new gpuTexture2d());
    gui_font.lut.reset(new gpuTexture2d());
    gui_font.atlas->setData(&imgFontAtlas);
    gui_font.lut->setData(&imgFontLookupTexture);
    gui_font.lut->setFilter(GPU_TEXTURE_FILTER_NEAREST);
}