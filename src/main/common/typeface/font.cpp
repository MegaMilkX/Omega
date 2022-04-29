#include "font.hpp"

#include "common/util/rect_pack.hpp"

#include <freetype/ftstroke.h>

extern FT_Library* s_ftlib;


const FontGlyph& Font::loadGlyph(uint32_t ch) {
    FT_Set_Char_Size(typeface->face, 0, font_height * 64.0f, dpi, dpi);

    FontGlyph& g = glyphs[ch];

    FT_UInt glyph_idx = FT_Get_Char_Index(typeface->face, ch);
    FT_Error err = FT_Load_Glyph(typeface->face, glyph_idx, FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL);
    assert(!err);
    if (typeface->face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        err = FT_Render_Glyph(typeface->face->glyph, FT_RENDER_MODE_NORMAL);
        assert(!err);
    }
    assert(typeface->face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

    size_t w = typeface->face->glyph->bitmap.width;
    size_t h = typeface->face->glyph->bitmap.rows;
    
    g.width = w;
    g.height = h;
    g.horiAdvance = typeface->face->glyph->advance.x;
    g.bearingX = typeface->face->glyph->bitmap_left;
    g.bearingY = typeface->face->glyph->bitmap_top;
    
    // TODO: ?
    return g;
}


Font::Font(Typeface* typeface, int font_height, int dpi)
: typeface(typeface), font_height(font_height), dpi(dpi) {
    FT_Set_Char_Size(typeface->face, 0, font_height * 64.0f, dpi, dpi);
    line_height = typeface->face->size->metrics.height / 64.0f;
}


int Font::getLineHeight() const {
    return line_height;
}
const FontGlyph& Font::getGlyph(uint32_t ch) {
    auto it = glyphs.find(ch);
    if (it == glyphs.end()) {
        return loadGlyph(ch);
    } else {
        return it->second;
    }
}

void Font::buildAtlas(ktImage* image, ktImage* lookup_texture) {
    const FT_Face& face = typeface->face;

    FT_Error err = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    assert(!err);
    err = FT_Set_Char_Size(face, 0, font_height * 64.0f, dpi, dpi);
    assert(!err);

    std::vector<RectPack::Rect> rects;
    rects.reserve(128);
    for (int i = 0; i < 128; ++i) {
        FT_UInt glyph_idx = FT_Get_Char_Index(face, i);
        FT_Error err = FT_Load_Glyph(face, glyph_idx, FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL);
        assert(!err);
        if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
            err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
            assert(!err);
        }
        assert(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);
        size_t w = face->glyph->bitmap.width;
        size_t h = face->glyph->bitmap.rows;
        size_t bmp_byte_len = w * h; // 1 bpp

        RectPack::Rect r;
        r.w = w;
        r.h = h;
        rects.push_back(r);
    }

    RectPack packer;
    RectPack::Rect image_rect = packer.pack(
        rects.data(), rects.size(), 1,
        RectPack::MAXSIDE, RectPack::POWER_OF_TWO
    );

    image->reserve(image_rect.w, image_rect.h, 1);
    for (int i = 0; i < 128; ++i) {
        FT_UInt glyph_idx = FT_Get_Char_Index(face, i);
        FT_Error err = FT_Load_Glyph(face, glyph_idx, FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL);
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
            err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
            assert(!err);
        }
        assert(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);
        size_t w = face->glyph->bitmap.width;
        size_t h = face->glyph->bitmap.rows;
        size_t bmp_byte_len = w * h; // 1 bpp

        image->blit(
            face->glyph->bitmap.buffer, w, h, 1, rects[i].x, rects[i].y
        );
    }

    // Build uv lookup texture
    std::vector<gfxm::vec2> lookup;
    for (int i = 0; i < rects.size(); ++i) {

        gfxm::vec2 l[] = {
            gfxm::vec2(rects[i].x / image_rect.w,      (rects[i].y + rects[i].h) / image_rect.h),
            gfxm::vec2((rects[i].x + rects[i].w) / image_rect.w,       (rects[i].y + rects[i].h) / image_rect.h),
            gfxm::vec2((rects[i].x + rects[i].w) / image_rect.w,       rects[i].y / image_rect.h),
            gfxm::vec2(rects[i].x / image_rect.w,      rects[i].y / image_rect.h)
        };/*
        gfxm::vec2 l[] = {
            gfxm::vec2(.0f,      1.f),
            gfxm::vec2(1.f,       1.f),
            gfxm::vec2(1.f,       .0f),
            gfxm::vec2(.0f,      .0f)
        };*/
        lookup.insert(lookup.end(), l, l + sizeof(l) / sizeof(l[0]));
    }
    int lookup_texture_width = lookup.size();
    lookup_texture->setData(lookup.data(), lookup_texture_width, 1, 2, IMAGE_CHANNEL_FLOAT);
}