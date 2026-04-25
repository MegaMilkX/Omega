#include "font.hpp"

#include <cctype>
#include <memory>

#include "rect_pack.hpp"

#include <freetype/ftstroke.h>

extern FT_Library* s_ftlib;


static uint32_t utf8_next(const char*& ptr, const char* end) {
    uint8_t c = (uint8_t)*ptr++;
    if (c < 0x80) return c;
    int extra = (c < 0xE0) ? 1 : (c < 0xF0) ? 2 : 3;
    uint32_t cp = c & (0x3F >> extra);
    while (extra-- && ptr < end)
        cp = (cp << 6) | ((uint8_t)*ptr++ & 0x3F);
    return cp;
}

FontGlyph& Font::loadGlyph(uint32_t ch) {
    FT_Set_Char_Size(typeface->face, 0, font_height * 64.0f, dpi, dpi);

    uint32_t cache_idx = glyph_cache.size();
    glyphs[ch] = cache_idx;
    FontGlyph& g = glyph_cache.emplace_back();

    FT_UInt glyph_idx = FT_Get_Char_Index(typeface->face, ch);
    FT_Error err = FT_Load_Glyph(typeface->face, glyph_idx, FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT);
    assert(!err);
    if (typeface->face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        err = FT_Render_Glyph(typeface->face->glyph, FT_RENDER_MODE_LIGHT);
        assert(!err);
    }
    assert(typeface->face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

    size_t w = typeface->face->glyph->bitmap.width;
    size_t h = typeface->face->glyph->bitmap.rows;
    
    g.glyph_idx = glyph_idx;
    g.cache_idx = cache_idx;
    g.width = w;
    g.height = h;
    g.horiAdvance = typeface->face->glyph->advance.x;
    g.bearingX = typeface->face->glyph->bitmap_left;
    g.bearingY = typeface->face->glyph->bitmap_top;
    
    // TODO: ?
    is_texture_data_stale = true;
    //updateTextures();
    return g;
}

void Font::updateTextures() {
    /*
    if (texture_data == nullptr) {
        texture_data.reset(new FontTextureData);

        ktImage imgFontAtlas;
        ktImage imgFontLookupTexture;
        buildAtlas(&imgFontAtlas, &imgFontLookupTexture);
        
        texture_data->atlas.reset(new gpuTexture2d());
        texture_data->lut.reset(new gpuTexture2d());
        texture_data->atlas->setData(&imgFontAtlas);
        texture_data->lut->setData(&imgFontLookupTexture);
        texture_data->lut->setFilter(GPU_TEXTURE_FILTER_NEAREST);
    }*/
    if (texture_data == nullptr) {
        texture_data.reset(new FontTextureData);
        texture_data->atlas.reset(new gpuTexture2d());
        texture_data->lut.reset(new gpuTexture2d());
    }

    ktImage imgAtlas;
    ktImage imgLut;
    buildAtlas(&imgAtlas, &imgLut);
    texture_data->atlas->setData(&imgAtlas);
    texture_data->lut->setData(&imgLut);
    texture_data->lut->setFilter(GPU_TEXTURE_FILTER_NEAREST);

    is_texture_data_stale = false;
}


Font::Font(const std::shared_ptr<Typeface>& typeface, int font_height, int dpi)
: typeface(typeface), font_height(font_height), dpi(dpi) {
    init(typeface, font_height, dpi);
    LOG("Created font " << typeface->filename << ", height: " << font_height << ", dpi: " << dpi);

    loadRange(0, 255);
}

void Font::init(const std::shared_ptr<Typeface>& typeface, int font_height, int dpi) {
    glyphs.clear();

    this->typeface = typeface;
    this->font_height = font_height;
    this->dpi = dpi;

    FT_Set_Char_Size(typeface->face, 0, font_height * 64.0f, dpi, dpi);
    //FT_Set_Pixel_Sizes(typeface->face, 0, 23);
    line_height = typeface->face->size->metrics.height / 64.0f;
    line_gap = (typeface->face->size->metrics.height - typeface->face->ascender - abs(typeface->face->descender)) / 64.0f;
    ascender = typeface->face->size->metrics.ascender / 64.0f;
    descender = abs(typeface->face->size->metrics.descender) / 64.0f;
    /*
    ascender = roundf(FT_MulFix(
        typeface->face->size->metrics.ascender,
        typeface->face->size->metrics.y_scale
    ) / 64.0f);
    descender = roundf(FT_MulFix(
        typeface->face->size->metrics.descender,
        typeface->face->size->metrics.y_scale
    ) / 64.0f);*/
}
void Font::loadRange(uint32_t begin, uint32_t end) {
    for (uint32_t ch = begin; ch < end; ++ch) {
        getGlyph(ch);
    }
}

int Font::getHeight() const {
    return font_height;
}
int Font::getDpi() const {
    return dpi;
}
int Font::getLineHeight() const {
    return line_height;
}
int Font::getLineGap() const {
    return line_gap;
}
int Font::getAscender() const {
    return ascender;
}
int Font::getDescender() const {
    return descender;
}
FontGlyph Font::getGlyph(uint32_t ch) {
    auto it = glyphs.find(ch);
    if (it == glyphs.end()) {
        return loadGlyph(ch);
    } else {
        return glyph_cache[it->second];
    }
}
FontGlyph& Font::getGlyphRef(uint32_t ch) {
    auto it = glyphs.find(ch);
    if (it == glyphs.end()) {
        return loadGlyph(ch);
    } else {
        return glyph_cache[it->second];
    }
}

FontTextureData* Font::getTextureData() {
    if (!texture_data || is_texture_data_stale) {
        updateTextures();
    }
    /*
    if (texture_data == nullptr) {
        texture_data.reset(new FontTextureData);

        ktImage imgFontAtlas;
        ktImage imgFontLookupTexture;
        buildAtlas(&imgFontAtlas, &imgFontLookupTexture);
        
        texture_data->atlas.reset(new gpuTexture2d());
        texture_data->lut.reset(new gpuTexture2d());
        texture_data->atlas->setData(&imgFontAtlas);
        texture_data->lut->setData(&imgFontLookupTexture);
        texture_data->lut->setFilter(GPU_TEXTURE_FILTER_NEAREST);
    }*/
    return texture_data.get();
}

void Font::buildAtlas(ktImage* image, ktImage* lookup_texture) {
    const FT_Face& face = typeface->face;

    FT_Error err = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    assert(!err);
    err = FT_Set_Char_Size(face, 0, font_height * 64.0f, dpi, dpi);
    assert(!err);

    const FT_Int32 LOAD_FLAGS = FT_LOAD_COLOR | FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT;

    std::vector<RectPack::Rect> rects;
    rects.reserve(128);
    for (int i = 0; i < glyph_cache.size(); ++i) {
        const auto& cached_glyph = glyph_cache[i];
        uint32_t glyph_idx = cached_glyph.glyph_idx;
        //FT_UInt glyph_idx = FT_Get_Char_Index(face, codepoint);
        FT_Error err = FT_Load_Glyph(face, glyph_idx, LOAD_FLAGS);
        assert(!err);
        if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
            err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LIGHT);
            assert(!err);
        }
        assert(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);
        size_t w = face->glyph->bitmap.width;
        size_t h = face->glyph->bitmap.rows;

        RectPack::Rect& r = rects.emplace_back();
        r.id = cached_glyph.cache_idx; // same as i
        r.w = w;
        r.h = h;
    }

    // TODO: border should be readable by users of the font
    constexpr int border = 1;
    RectPack packer;
    RectPack::Rect image_rect = packer.pack(
        rects.data(), rects.size(), border,
        RectPack::MAXSIDE, RectPack::POWER_OF_TWO
    );

    image->reserve(image_rect.w, image_rect.h, 1);
    for (int i = 0; i < rects.size(); ++i) {
        uint32_t glyph_idx = glyph_cache[rects[i].id].glyph_idx;
        FT_Error err = FT_Load_Glyph(face, glyph_idx, LOAD_FLAGS);
        assert(!err);
        FT_BitmapGlyph bitmapGlyph;
        if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
            /* DO THIS IF U WANT BORDERS, GLYPH BITMAP RECT BECOMS BIGGER, NEED TO ADJUST FOR THAT
            FT_Stroker stroker;
            FT_Stroker_New(*s_ftlib, &stroker);
            FT_Stroker_Set(stroker, 2 * 64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

            FT_Glyph glph;
            FT_Get_Glyph(face->glyph, &glph);
            FT_Glyph_StrokeBorder(&glph, stroker, false, true);
            FT_Glyph_To_Bitmap(&glph, FT_RENDER_MODE_NORMAL, 0, true);
            bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glph);
            */
            err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LIGHT);
            assert(!err);
        }
        assert(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);
        size_t w = face->glyph->bitmap.width;
        size_t h = face->glyph->bitmap.rows;

        image->blit(
            face->glyph->bitmap.buffer, w, h, 1, rects[i].x, rects[i].y
        );
    }

    // Build uv lookup texture
    for (int i = 0; i < rects.size(); ++i) {
        auto& rc = rects[i];
        rc.x -= border;
        rc.y -= border;
        rc.w += border * 2;
        rc.h += border * 2;
    }
    std::vector<gfxm::vec2> lookup(glyph_cache.size() * 4);
    for (int i = 0; i < rects.size(); ++i) {
        gfxm::vec2 _min = gfxm::vec2(
            rects[i].x / image_rect.w,
            rects[i].y / image_rect.h
        );
        gfxm::vec2 _max = gfxm::vec2(
            (rects[i].x + rects[i].w) / image_rect.w,
            (rects[i].y + rects[i].h) / image_rect.h
        );

        gfxm::vec2 l[] = {
            gfxm::vec2(_min.x, _max.y),
            gfxm::vec2(_max.x, _max.y),
            gfxm::vec2(_max.x, _min.y),
            gfxm::vec2(_min.x, _min.y)
        };

        auto& g = glyph_cache[rects[i].id];
        g.uv_rect.min = gfxm::vec2(_min.x, _max.y);
        g.uv_rect.max = gfxm::vec2(_max.x, _min.y);
        g.atlas_px_min_x = rects[i].x;
        g.atlas_px_min_y = rects[i].y + rects[i].h;
        g.atlas_px_max_x = rects[i].x + rects[i].w;
        g.atlas_px_max_y = rects[i].y;

        lookup[g.cache_idx * 4] = l[0];
        lookup[g.cache_idx * 4 + 1] = l[1];
        lookup[g.cache_idx * 4 + 2] = l[2];
        lookup[g.cache_idx * 4 + 3] = l[3];
        //lookup.insert(lookup.end(), l, l + sizeof(l) / sizeof(l[0]));
    }
    if (lookup_texture) {
        int lookup_texture_width = lookup.size();
        lookup_texture->setData(lookup.data(), lookup_texture_width, 1, 2, IMAGE_CHANNEL_FLOAT);
    }
}

int Font::findCursorPos(const char* str, int str_len, float pointer_x, float max_width, float* out_screen_x) {
    int line_offset = getLineHeight();
    int hori_advance = 0;
    int max_hori_advance = 0;

    int adv_space = getGlyph('\s').horiAdvance;
    int adv_tab = adv_space * 8;

    int ret_cursor = 0;
    auto putGlyph = [&line_offset, &hori_advance, &max_hori_advance, &pointer_x, &ret_cursor, out_screen_x]
    (const FontGlyph& g, char ch, int i)->bool {
        if (hori_advance + g.horiAdvance / 64 > pointer_x) {
            ret_cursor = gfxm::_max(i, 0);
            (*out_screen_x) = (float)hori_advance;
            return true;
        }
        hori_advance += g.horiAdvance / 64;
        max_hori_advance = gfxm::_max(max_hori_advance, hori_advance);
        return false;
    };
    auto putNewline = [&line_offset, &hori_advance, this]() {
        line_offset += getLineHeight();
        hori_advance = 0;
    };

    for (int i = 0; i < str_len; ++i) {
        char ch = str[i];
        if (hori_advance >= max_width && max_width > .0f) {
            line_offset += getLineHeight();
            hori_advance = 0;
        }
        if (ch == '\n') {
            putNewline();
            continue;
        } else if(ch == '\t') {
            int tab_reminder = hori_advance % (adv_tab / 64);
            int adv = adv_tab / 64 - tab_reminder;
            hori_advance += adv;
        } else if(ch == '#') {
            int characters_left = str_len - i - 1;
            if (characters_left >= 8) {
                i += 8;
                continue;
            }
        } else if(isspace(ch)) {
            const auto& g = getGlyph(ch);
            if (putGlyph(g, ch, i)) {
                goto abort;
            }
        } else {
            int tok_pos = i;
            int tok_len = 0;
            for (int j = i; j < str_len; ++j) {
                ch = str[j];
                if (isspace(ch)) {
                    break;
                }
                ++tok_len;
            }
            int word_hori_advance = 0; 
            for (int j = tok_pos; j < tok_pos + tok_len; ++j) {
                ch = str[j];
                const auto& g = getGlyph(ch);
                word_hori_advance += g.horiAdvance / 64;
            }
            if (hori_advance + word_hori_advance >= max_width && max_width > .0f) {
                putNewline();
            }
            for (int j = tok_pos; j < tok_pos + tok_len; ++j) {
                ch = str[j];
                const auto& g = getGlyph(ch);
                if (putGlyph(g, ch, j)) {
                    goto abort;
                }
            }            

            i += tok_len - 1;
        }
    }

    ret_cursor = str_len;
    (*out_screen_x) = (float)max_hori_advance;

abort:
    return ret_cursor;
}


std::shared_ptr<Font> fontGet(const char* typeface_name, int height, int dpi) {
    std::shared_ptr<Typeface> typeface = typefaceGet(typeface_name);
    if (!typeface) {
        return 0;
    }

    return typeface->getFont(height, dpi);
}
