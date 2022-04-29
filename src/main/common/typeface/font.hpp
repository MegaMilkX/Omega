#pragma once

#include "typeface.hpp"


struct FontGlyph {
    float width;
    float height;
    float horiAdvance;
    float bearingX;
    float bearingY;

    uint64_t cache_id;
};

class Font {
    Typeface* typeface = 0;
    int font_height = 0;
    int dpi = 0;
    int line_height = 0;
    std::unordered_map<uint32_t, FontGlyph> glyphs;

    const FontGlyph& loadGlyph(uint32_t ch);
public:
    Font() {}
    Font(Typeface* typeface, int font_height, int dpi);

    int getLineHeight() const;
    const FontGlyph& getGlyph(uint32_t ch);

    void buildAtlas(ktImage* image, ktImage* lookup_texture);
};