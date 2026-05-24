#pragma once

#include <stack>
#include "math/gfxm.hpp"
#include "typeface/font.hpp"


struct TextLayout {
    enum HALIGN {
        HALIGN_LEFT,
        HALIGN_CENTER,
        HALIGN_RIGHT
    };
    enum VALIGN {
        VALIGN_TOP,
        VALIGN_CENTER,
        VALIGN_BOTTOM
    };
    enum SPACE {
        Y_UP,
        Y_DOWN
    };

    struct Quad {
        gfxm::vec3 pos[4];
        gfxm::vec2 uv[4];
        uint32_t rgba[4];
        float lut_values[4];
    };
    struct GlyphInstance {
        FontGlyph* glyph = nullptr;
        gfxm::rect glyph_rect;
        gfxm::rect uv_rect;
        float lut_values[4] = { 0 };
        uint32_t color = 0xFFFFFFFF;
        int line_idx = 0;
        int x_left_side = 0;
        int x_midpoint = 0; // hit testing
        int raw_idx = 0;
        bool renderable = true;

        Quad makeQuad() const {
            const auto& grc = glyph_rect;
            const auto& uvrc = uv_rect;

            Quad q;
            q.pos[0] = gfxm::vec3( grc.min.x, grc.min.y, .0f );
            q.pos[1] = gfxm::vec3( grc.max.x, grc.min.y, .0f );
            q.pos[2] = gfxm::vec3( grc.min.x, grc.max.y, .0f );
            q.pos[3] = gfxm::vec3( grc.max.x, grc.max.y, .0f );
            q.uv[0] = gfxm::vec2( uvrc.min.x, uvrc.min.y );
            q.uv[1] = gfxm::vec2( uvrc.max.x, uvrc.min.y );
            q.uv[2] = gfxm::vec2( uvrc.min.x, uvrc.max.y );
            q.uv[3] = gfxm::vec2( uvrc.max.x, uvrc.max.y );
            q.rgba[0] = color;
            q.rgba[1] = color;
            q.rgba[2] = color;
            q.rgba[3] = color;
            q.lut_values[0] = lut_values[0];
            q.lut_values[1] = lut_values[1];
            q.lut_values[2] = lut_values[2];
            q.lut_values[3] = lut_values[3];

            return q;
        }
    };
    struct Line {
        int begin = 0, end = 0;
        int raw_begin = 0, raw_end = 0;
        int y_baseline = 0;
        int bounding_width = 0;
        int hori_align_offset = 0;
    };
    struct Span {
        int line_idx = 0;
        uint32_t color = 0xFFFFFFFF;
        gfxm::rect rc;
    };
    struct Range {
        int begin;
        int end;
        uint32_t id;
    };

    // font derived
    int space_width = 0;
    int tab_width = 0;
    int line_height = 0;
    int ascender = 0;
    int descender = 0;
    int line_gap = 0;

    // input
    SPACE space = Y_DOWN;
    HALIGN hori_align = HALIGN::HALIGN_LEFT;
    VALIGN vert_align = VALIGN::VALIGN_TOP;
    int pad_left = 0;
    int pad_right = 0;
    int pad_top = 0;
    int pad_bottom = 0;
    uint32_t base_color = 0xFFFFFFFF;
    std::vector<Range> user_spans;

    // output
    std::vector<Line> lines;
    std::vector<GlyphInstance> glyphs;
    std::vector<Span> spans;
    int bounding_width = 0, bounding_height = 0;
    int bounding_width_no_pad = 0, bounding_height_no_pad = 0;
    int box_height = 0;

    void _beginSpanQuad(int line_idx, uint32_t col, int x);
    void _endSpanQuad(int x);
    int _calcVertOffset(VALIGN valign, int height);

    void clearRanges();
    void addRange(int begin, int end, uint32_t id);

    void build(const std::string& str, Font* font, int max_width = -1);
    void alignHorizontal(HALIGN halign, int width);
    void alignVertical(VALIGN valign, int height);
    void padHorizontal(int left, int right);
    void padVertical(int top, int bottom);

    int hitTest(int x, int y);
    Line* hitTestLine(int x, int y);
};

