#pragma once

#include "typeface.hpp"
#include <memory>
#include "gpu/gpu_texture_2d.hpp"


struct FontGlyph {
    gfxm::rect uv_rect;
    int atlas_px_min_x = 0;
    int atlas_px_min_y = 0;
    int atlas_px_max_x = 0;
    int atlas_px_max_y = 0;
    float width;
    float height;
    float horiAdvance;
    float bearingX;
    float bearingY;
    uint32_t glyph_idx = 0;
    uint32_t cache_idx = 0;
};

struct FontTextureData {
    std::unique_ptr<gpuTexture2d> atlas;
    std::unique_ptr<gpuTexture2d> lut;
};

class Font {
    std::shared_ptr<Typeface> typeface = 0;
    int font_height = 0;
    int dpi = 0;
    int line_height = 0;
    int line_gap = 0;
    int ascender = 0;
    int descender = 0;
    std::unordered_map<uint32_t, int> glyphs;
    std::vector<FontGlyph> glyph_cache;
    std::unique_ptr<FontTextureData> texture_data;
    bool is_texture_data_stale = true;

    FontGlyph& loadGlyph(uint32_t ch);
    void updateTextures();
public:
    Font() {}
    Font(const std::shared_ptr<Typeface>& typeface, int font_height, int dpi);

    void init(const std::shared_ptr<Typeface>& typeface, int font_height, int dpi);

    void loadRange(uint32_t begin, uint32_t end);

    const Typeface* getTypeface() const {
        return typeface.get();
    }

    int getHeight() const;
    int getDpi() const;
    int getLineHeight() const;
    int getLineGap() const;
    int getAscender() const;
    int getDescender() const;
    FontGlyph getGlyph(uint32_t ch);
    FontGlyph& getGlyphRef(uint32_t ch);

    FontTextureData* getTextureData();

    void buildAtlas(ktImage* image, ktImage* lookup_texture);
    int findCursorPos(const char* str, int str_len, float pointer_x, float max_width, float* out_screen_x);
};

std::shared_ptr<Font> fontGet(const char* typeface_name, int height, int dpi = 72);
