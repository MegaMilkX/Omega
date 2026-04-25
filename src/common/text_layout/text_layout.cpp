#include "text_layout.hpp"

static uint32_t utf8_next(const char*& ptr, const char* end) {
    uint8_t c = (uint8_t)*ptr++;
    if (c < 0x80) return c;
    int extra = (c < 0xE0) ? 1 : (c < 0xF0) ? 2 : 3;
    uint32_t cp = c & (0x3F >> extra);
    while (extra-- && ptr < end)
        cp = (cp << 6) | ((uint8_t)*ptr++ & 0x3F);
    return cp;
}

void TextLayout::build(const std::string& str, Font* font, int max_width) {
    lines.clear();
    glyphs.clear();

    std::stack<uint32_t> color_stack;

    int padded_width = -1;
    if (max_width >= 0) {
        padded_width = max_width - (pad_left + pad_right);
    }

    const int SPACES_PER_TAB = 4;
    const int ATLAS_PAD = 1;

    const int space_width = font->getGlyph(' ').horiAdvance / 64;
    const int tab_width = space_width * SPACES_PER_TAB;
    const int line_height = font->getLineHeight();
    const int descender = font->getDescender();

    Line* line = &lines.emplace_back();
    line->begin = 0;

    int n_line = 0;
    int hori_advance = 0;
    int max_hori_advance = 0;
    int next_tab_stop = 0;
    const char* pbegin = str.data();
    const char* pend = str.data() + str.size();
    const char* pcur = pbegin;
    while (pcur != pend) {
        const char* pcur_word = pcur;
        uint32_t ch = utf8_next(pcur, pend);
        if (ch == '\n') {
            // Line break
            line->bounding_width = hori_advance;
            line->end = glyphs.size();
            line = &lines.emplace_back();
            line->begin = glyphs.size();
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
        const char* pcur_word_begin = pcur_word;
        while(pcur_word != pend) {
            const char* pcur_tmp = pcur_word;
            uint32_t ch = utf8_next(pcur_tmp, pend);
            if (ch < 255 && isspace(ch)) {
                break;
            }
            const auto& g = font->getGlyph(ch);
            int glyph_advance = g.horiAdvance / 64;
            /*if (ch == 0x02 || ch == 0x03) {
                glyph_advance = 0;
            }*/
            word_advance += glyph_advance;
            ++word_len;
            pcur_word = pcur_tmp;
        }
        const char* pcur_word_end = pcur_word;
        pcur_word = pcur_word_begin;

        if (padded_width != -1 && hori_advance + word_advance > padded_width && hori_advance != 0) {
            // Line break
            line->bounding_width = hori_advance;
            line->end = glyphs.size();
            line = &lines.emplace_back();
            line->begin = glyphs.size();
            ++n_line;
            hori_advance = 0;
            next_tab_stop = tab_width * (1 + hori_advance / tab_width);
        }

        int line_offset = line_height * (n_line + 1) - descender;

        while(pcur_word < pcur_word_end) {
            ch = utf8_next(pcur_word, pend);
            if (ch == 0x02) { // STX
                uint32_t col = 0;
                const char* begin = pcur_word;
                const char* end = begin + 4;
                int ib = 0;
                for (const char* pch = begin; pch < end && pch < pend; ++pch) {
                    uint8_t byte = uint8_t(*pch);
                    uint32_t comp = uint32_t(byte) << (ib * 8);
                    col |= comp;
                    ++ib;
                }
                color_stack.push(col);
                pcur_word = end;
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
            grc.min.x = pad_left + hori_advance + x_ofs - float(ATLAS_PAD);
            grc.max.x = pad_left + hori_advance + g.width + x_ofs + float(ATLAS_PAD);
            grc.min.y = -pad_top + 0 - y_ofs - line_offset - float(ATLAS_PAD);
            grc.max.y = -pad_top + g.height - y_ofs - line_offset + float(ATLAS_PAD);

            auto& uvrc = out_glyph.uv_rect;
            uvrc.min.x = .0f;
            uvrc.max.x = 1.f;
            uvrc.min.y = .0f;
            uvrc.max.y = 1.f;

            uint32_t color = base_color;
            if (!color_stack.empty()) {
                color = color_stack.top();
            }
            out_glyph.color = color;

            auto& lutv = out_glyph.lut_values;
            lutv[0] = g.cache_idx * 4;
            lutv[1] = g.cache_idx * 4 + 1;
            lutv[2] = g.cache_idx * 4 + 3;
            lutv[3] = g.cache_idx * 4 + 2;

            hori_advance += g.horiAdvance / 64;
            max_hori_advance = gfxm::_max(hori_advance, max_hori_advance);
        }
        pcur = pcur_word_end;
    }
    line->bounding_width = hori_advance;
    line->end = glyphs.size();
    bounding_width_no_pad = max_hori_advance;
    bounding_height_no_pad = line_height * (n_line + 1);
    bounding_width = bounding_width_no_pad + pad_left + pad_right;
    bounding_height = bounding_height_no_pad + pad_top + pad_bottom;
}

void TextLayout::alignHorizontal(HALIGN halign, int box_width) {
    float align_mul = .0f;
    switch (halign) {
    case HALIGN_LEFT: break;
    case HALIGN_CENTER: align_mul = .5f; break;
    case HALIGN_RIGHT: align_mul = 1.f; break;
    default: assert(false);
    }
    const int halign_max_width = box_width >= 0 ? box_width : bounding_width_no_pad;
    for (int i = 0; i < lines.size(); ++i) {
        auto& line = lines[i];
        const int offs = (halign_max_width - line.bounding_width) * align_mul;
        for (int j = line.begin; j < line.end; ++j) {
            auto& g = glyphs[j];
            g.glyph_rect.min.x += offs;
            g.glyph_rect.max.x += offs;
        }
    }
}

void TextLayout::alignVertical(VALIGN valign, int box_height) {
    float align_mul = .0f;
    switch (valign) {
    case VALIGN_TOP: break;
    case VALIGN_CENTER: align_mul = .5f; break;
    case VALIGN_BOTTOM: align_mul = 1.f; break;
    default: assert(false);
    }
    const int valign_max_height = box_height;
    const int offs = (valign_max_height - bounding_height_no_pad) * align_mul;
    for (int i = 0; i < lines.size(); ++i) {
        auto& line = lines[i];
        for (int j = line.begin; j < line.end; ++j) {
            auto& g = glyphs[j];
            g.glyph_rect.min.y -= offs;
            g.glyph_rect.max.y -= offs;
        }
    }
}
void TextLayout::padHorizontal(int left, int right) {
    for (int i = 0; i < glyphs.size(); ++i) {
        auto& g = glyphs[i];
        g.glyph_rect.min.x += left;
        g.glyph_rect.max.x += left;
    }
    bounding_width += (left + right);
}
void TextLayout::padVertical(int top, int bottom) {
    for (int i = 0; i < glyphs.size(); ++i) {
        auto& g = glyphs[i];
        g.glyph_rect.min.y -= top;
        g.glyph_rect.max.y -= top;
    }
    bounding_height += (top + bottom);
}

