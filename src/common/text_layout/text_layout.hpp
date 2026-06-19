#pragma once

#include <stack>
#include "math/gfxm.hpp"
#include "typeface/font.hpp"


struct TextLayout {
    static constexpr uint32_t DIRTY_LINES       = 0x0001;
    static constexpr uint32_t DIRTY_WRAPPING    = 0x0002;
    static constexpr uint32_t DIRTY_GLYPHS      = 0x0004;
    // TODO: These are not implemented yet
    static constexpr uint32_t DIRTY_PADDING     = 0x0020;

    enum class BuildMode {
        TellWidth,
        TellHeight,
        Full
    };

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
        int raw_begin = 0, raw_end = 0;
        int decoded_begin = 0, decoded_end = 0;
        int glyph_begin = 0, glyph_end = 0;
        int y_baseline = 0;
        int bounding_width = 0;
        int hori_align_offset = 0;
        int x_end = 0;
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

    uint32_t dirty_flags = 0;

    SPACE space = Y_DOWN;
    uint32_t base_color = 0xFFFFFFFF;
    Font* font = nullptr;
    const char* string_view = nullptr;
    size_t string_len = 0;
    std::optional<int> width_constraint;
    std::optional<int> height_constraint;
    HALIGN hori_align = HALIGN::HALIGN_LEFT;
    VALIGN vert_align = VALIGN::VALIGN_TOP;
    int pad_left = 0;
    int pad_right = 0;
    int pad_top = 0;
    int pad_bottom = 0;
    std::vector<Range> user_spans;

    std::vector<uint32_t> string_decoded;

    // font derived
    int space_width = 0;
    int tab_width = 0;
    int line_height = 0;
    int ascender = 0;
    int descender = 0;
    int line_gap = 0;

    // output
    std::vector<Line> lines;
    std::vector<Line> lines_wrapped;
    std::vector<GlyphInstance> glyphs;
    std::vector<Span> spans;

    int bounding_width = 0, bounding_height = 0;
    int bounding_width_no_pad = 0, bounding_height_no_pad = 0;
    int box_height = 0;

    bool _is_space(uint32_t ch);

    void _beginSpanQuad(int line_idx, uint32_t col, int x);
    void _endSpanQuad(int x);
    int _calcVertOffset(VALIGN valign, int height);

    void clearRanges();
    void addRange(int begin, int end, uint32_t id);

    void setFont(Font* f);
    void setString(const char* str, size_t len);
    void setWidth(std::optional<int> w);
    void setHeight(std::optional<int> h);
    void setHAlign(HALIGN halign);
    void setVAlign(VALIGN valign);
    void setPadding(int left, int right, int top, int bottom);
    void build(BuildMode build_mode = BuildMode::Full);

    void build(const std::string& str, Font* font, int max_width = -1);
    void alignHorizontal(HALIGN halign, int width);
    void alignVertical(VALIGN valign, int height);
    void padHorizontal(int left, int right);
    void padVertical(int top, int bottom);

    int hitTest(int x, int y);
    Line* hitTestLine(int x, int y);
};

