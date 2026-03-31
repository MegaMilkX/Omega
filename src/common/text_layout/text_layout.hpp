#pragma once

#include <stack>
#include "math/gfxm.hpp"
#include "typeface/font.hpp"


struct TextLayout {
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
    std::vector<GlyphInstance> glyphs;
    int bounding_width = 0, bounding_height = 0;

    void build(const std::string& str, Font* font, int max_width = -1) {
        glyphs.clear();

        std::stack<uint32_t> color_stack;

        const int SPACES_PER_TAB = 4;
        const int ATLAS_PAD = 1;

        const int space_width = font->getGlyph(' ').horiAdvance / 64;
        const int tab_width = space_width * SPACES_PER_TAB;

        int n_line = 0;
        const int line_height = font->getLineHeight();
        const int descender = font->getDescender();
        int hori_advance = 0;
        int max_hori_advance = 0;
        int next_tab_stop = 0;
        for (int ich = 0; ich < str.size(); ++ich) {
            char ch = str[ich];
            if (ch == '\n') {
                ++n_line;
                hori_advance = 0;
                continue;
            }

            next_tab_stop = tab_width * (1 + hori_advance / tab_width);

            if (ch == '\t') {
                hori_advance = next_tab_stop;
                continue;
            }

            const auto& g = font->getGlyph(ch);
            //int y_ofs = g.height - g.bearingY;
            //int x_ofs = g.bearingX;
            int glyph_advance = g.horiAdvance / 64;

            if (isspace(ch)) {
                hori_advance += glyph_advance;
                max_hori_advance = gfxm::_max(hori_advance, max_hori_advance);
                continue;
            }

            int word_advance = 0;
            int word_len = 0;
            for (int k = ich; k < str.size(); ++k) {
                ch = str[k];
                if (isspace(ch)) {
                    break;
                }
                const auto& g = font->getGlyph(ch);
                int glyph_advance = g.horiAdvance / 64;
                word_advance += glyph_advance;
                ++word_len;
            }

            if (max_width != -1 && hori_advance + word_advance > max_width && hori_advance != 0) {
                ++n_line;
                hori_advance = 0;
                next_tab_stop = tab_width * (1 + hori_advance / tab_width);
            }

            int line_offset = line_height * (n_line + 1) - descender;

            for (int k = ich; k < ich + word_len; ++k) {
                ch = str[k];

                if (ch == 0x02) { // STX
                    uint32_t col = 0;
                    int begin = k + 1;
                    int end = begin + 4;
                    for (int icol = begin; icol < end && icol < str.size(); ++icol) {
                        int ib = icol - begin;
                        uint8_t byte = uint8_t(str[icol]);
                        uint32_t comp = uint32_t(byte) << (ib * 8);
                        col |= comp;
                    }
                    color_stack.push(col);
                    k = end - 1;
                    continue;
                }
                if (ch == 0x03) { // ETX
                    if(!color_stack.empty()) {
                        color_stack.pop();
                    }
                    continue;
                }

                const auto& g = font->getGlyph(ch);
                int y_ofs = g.height - g.bearingY;
                int x_ofs = g.bearingX;
                int glyph_advance = g.horiAdvance / 64;

                auto& out_glyph = glyphs.emplace_back();
                auto& grc = out_glyph.glyph_rect;
                grc.min.x = hori_advance + x_ofs - float(ATLAS_PAD);
                grc.max.x = hori_advance + g.width + x_ofs + float(ATLAS_PAD);
                grc.min.y = 0 - y_ofs - line_offset - float(ATLAS_PAD);
                grc.max.y = g.height - y_ofs - line_offset + float(ATLAS_PAD);

                auto& uvrc = out_glyph.uv_rect;
                uvrc.min.x = .0f;
                uvrc.max.x = 1.f;
                uvrc.min.y = .0f;
                uvrc.max.y = 1.f;

                uint32_t color = 0xFFFFFFFF;
                if (!color_stack.empty()) {
                    color = color_stack.top();
                }
                out_glyph.color = color;

                auto& lutv = out_glyph.lut_values;
                lutv[0] = ch * 4;
                lutv[1] = ch * 4 + 1;
                lutv[2] = ch * 4 + 3;
                lutv[3] = ch * 4 + 2;

                hori_advance += g.horiAdvance / 64;
                max_hori_advance = gfxm::_max(hori_advance, max_hori_advance);
            }
            ich += word_len - 1;
        }
        bounding_width = max_hori_advance;
        bounding_height = line_height * (n_line + 1);
    }
};

