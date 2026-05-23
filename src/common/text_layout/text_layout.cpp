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

void TextLayout::_beginSpanQuad(int line_idx, uint32_t col, int x) {
    Span sp;
    sp.line_idx = line_idx;
    sp.color = col;
    sp.rc.min.x = x;
    const int baseline = line_height * (line_idx + 1);
    sp.rc.min.y = baseline - ascender;
    sp.rc.max.y = baseline + descender;
    spans.push_back(sp);
}
void TextLayout::_endSpanQuad(int x) {
    spans.back().rc.max.x = x;
}
int TextLayout::_calcVertOffset(VALIGN valign, int box_height) {
    const int valign_max_height = box_height;
    const int text_height = bounding_height_no_pad;
    int offs = 0;

    switch (valign) {
    case VALIGN_TOP:
        return 0;
    case VALIGN_CENTER: {
        offs = (valign_max_height - text_height) / 2 - (line_height - ascender);
        break;
    }
    case VALIGN_BOTTOM: {
        offs = (valign_max_height - text_height);
        break;
    }
    default:
        assert(false);
        return 0;
    }
    return offs;
}

void TextLayout::clearRanges() {
    user_spans.clear();
}
void TextLayout::addRange(int begin, int end, uint32_t id) {
    if (end < begin) {
        std::swap(begin, end);
    }
    user_spans.push_back(Range{ begin, end, id });
}

void TextLayout::build(const std::string& str, Font* font, int max_width) {
    lines.clear();
    glyphs.clear();
    spans.clear();

    struct SpanInternal {
        int begin;
        int end;
        uint32_t color;
    };
    std::vector<SpanInternal> internal_spans;
    internal_spans.push_back(SpanInternal{ 0, int(str.size()), 0xFF000000 });
    
    for (int i = 0; i < user_spans.size(); ++i) {
        const auto& usp = user_spans[i];

        for (int j = 0; j < internal_spans.size(); ++j) {
            SpanInternal isp = internal_spans[j];
            SpanInternal intersection{
                gfxm::_max(isp.begin, usp.begin),
                gfxm::_min(isp.end, usp.end),
                0xFF0000FF
            };
            if (intersection.begin >= intersection.end) {
                continue;
            }

            internal_spans.erase(internal_spans.begin() + j);
            SpanInternal inter_left{ isp.begin, usp.begin, isp.color };
            SpanInternal inter_right{ usp.end, isp.end, isp.color };
            int offs = 0;
            if (inter_right.begin < inter_right.end) {
                internal_spans.insert(internal_spans.begin(), inter_right);
                ++offs;
            }
            internal_spans.insert(internal_spans.begin(), SpanInternal{ usp.begin, usp.end, usp.id });
            if (inter_left.begin < inter_left.end) {
                internal_spans.insert(internal_spans.begin(), inter_left);
                ++offs;
            }
            j += offs;
        }
    }
    std::sort(internal_spans.begin(), internal_spans.end(), [](const SpanInternal& l, const SpanInternal& r) {
        return l.begin < r.begin;
    });

    auto fn_findSpan = [&internal_spans](int current, int ch_idx)->int {
        for (int i = current; i < internal_spans.size(); ++i) {
            if (internal_spans[i].begin <= ch_idx && internal_spans[i].end > ch_idx) {
                return i;
            }
        }
        return current;
    };

    std::stack<uint32_t> color_stack;

    int padded_width = -1;
    if (max_width >= 0) {
        padded_width = max_width - (pad_left + pad_right);
    }

    const int SPACES_PER_TAB = 4;
    const int ATLAS_PAD = 1;

    space_width = font->getGlyph(' ').horiAdvance / 64;
    tab_width = space_width * SPACES_PER_TAB;
    line_height = font->getLineHeight();
    line_gap = font->getLineGap();
    ascender = font->getAscender();
    descender = font->getDescender();

    Line* line = &lines.emplace_back();
    line->begin = 0;
    line->raw_begin = 0;

    int n_line = 0;
    int hori_advance = 0;
    int max_hori_advance = 0;
    int next_tab_stop = 0;

    int span_idx = 0;
    _beginSpanQuad(n_line, internal_spans[span_idx].color, hori_advance);

    const char* pbegin = str.data();
    const char* pend = str.data() + str.size();
    const char* pcur = pbegin;
    int raw_index = 0;
    while (pcur != pend) {
        const char* pcur_word = pcur;
        raw_index = pcur - pbegin;
        uint32_t ch = utf8_next(pcur, pend);
        int next_raw_index = pcur - pbegin;
        int next_span = fn_findSpan(span_idx, raw_index);
        if (span_idx != next_span) {
            _endSpanQuad(hori_advance);
            span_idx = next_span;
            _beginSpanQuad(n_line, internal_spans[span_idx].color, hori_advance);
        }
        if (ch == '\n') {
            // Line break
            auto& out_glyph = glyphs.emplace_back();
            out_glyph.x_midpoint = hori_advance + space_width / 2;
            out_glyph.raw_idx = raw_index;
            out_glyph.renderable = false;

            _endSpanQuad(hori_advance + space_width);
            line->bounding_width = hori_advance;
            line->end = glyphs.size() - 1;
            line->raw_end = raw_index;
            line = &lines.emplace_back();
            line->begin = glyphs.size();
            line->raw_begin = next_raw_index;
            ++n_line;
            hori_advance = 0;
            _beginSpanQuad(n_line, internal_spans[span_idx].color, hori_advance);
            continue;
        }

        next_tab_stop = tab_width * (1 + hori_advance / tab_width);

        if (ch == '\t') {
            auto& out_glyph = glyphs.emplace_back();
            out_glyph.x_midpoint = hori_advance + (next_tab_stop - hori_advance) / 2;
            out_glyph.raw_idx = raw_index;
            out_glyph.renderable = false;

            hori_advance = next_tab_stop;
            continue;
        }

        const auto& g = font->getGlyph(ch);
        int glyph_advance = g.horiAdvance / 64;

        if (ch <= 255 && isspace(ch)) {
            auto& out_glyph = glyphs.emplace_back();
            out_glyph.x_midpoint = hori_advance + glyph_advance / 2;
            out_glyph.raw_idx = raw_index;
            out_glyph.renderable = false;

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
            if (ch <= 255 && isspace(ch)) {
                break;
            }
            const auto& g = font->getGlyph(ch);
            int glyph_advance = g.horiAdvance / 64;
            word_advance += glyph_advance;
            ++word_len;
            pcur_word = pcur_tmp;
        }
        const char* pcur_word_end = pcur_word;
        pcur_word = pcur_word_begin;

        if (padded_width != -1 && hori_advance + word_advance > padded_width && hori_advance != 0) {
            // Line break (word wrap)
            _endSpanQuad(hori_advance);
            line->bounding_width = hori_advance;
            line->end = glyphs.size();
            line->raw_end = pcur_word_begin - pbegin;
            line = &lines.emplace_back();
            line->begin = glyphs.size();
            line->raw_begin = pcur_word_begin - pbegin;
            ++n_line;
            hori_advance = 0;
            next_tab_stop = tab_width * (1 + hori_advance / tab_width);
            _beginSpanQuad(n_line, internal_spans[span_idx].color, hori_advance);
        }

        int line_offset = line_height * (n_line + 1);

        while(pcur_word < pcur_word_end) {
            raw_index = pcur_word - pbegin;
            ch = utf8_next(pcur_word, pend);
            int next_raw_index = pcur_word - pbegin;
            int next_span = fn_findSpan(span_idx, raw_index);
            if (span_idx != next_span) {
                _endSpanQuad(hori_advance);
                span_idx = next_span;
                _beginSpanQuad(n_line, internal_spans[span_idx].color, hori_advance);
            }
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
            out_glyph.x_midpoint = hori_advance + glyph_advance / 2;
            out_glyph.raw_idx = raw_index;

            auto& grc = out_glyph.glyph_rect;
            grc.min.x = pad_left + hori_advance + x_ofs - float(ATLAS_PAD);
            grc.max.x = pad_left + hori_advance + g.width + x_ofs + float(ATLAS_PAD);
            grc.min.y = -pad_top + 0 - y_ofs - line_offset - float(ATLAS_PAD);
            grc.max.y = -pad_top + g.height - y_ofs - line_offset + float(ATLAS_PAD);            
            if (space == Y_DOWN) {
                std::swap(grc.min.y, grc.max.y);
                grc.min.y = -grc.min.y;
                grc.max.y = -grc.max.y;
            }

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
            if (space == Y_UP) {
                lutv[0] = g.cache_idx * 4 + 0;
                lutv[1] = g.cache_idx * 4 + 1;
                lutv[2] = g.cache_idx * 4 + 3;
                lutv[3] = g.cache_idx * 4 + 2;
            } else if(space == Y_DOWN) {
                lutv[0] = g.cache_idx * 4 + 3;
                lutv[1] = g.cache_idx * 4 + 2;
                lutv[2] = g.cache_idx * 4 + 0;
                lutv[3] = g.cache_idx * 4 + 1;
            } else {
                assert(false);
            }

            hori_advance += glyph_advance;
            max_hori_advance = gfxm::_max(hori_advance, max_hori_advance);
        }
        pcur = pcur_word_end;
    }
    _endSpanQuad(hori_advance);

    line->bounding_width = hori_advance;
    line->end = glyphs.size();
    line->raw_end = pend - pbegin;
    bounding_width_no_pad = max_hori_advance;
    bounding_height_no_pad = line_height * (n_line + 1);
    bounding_width = bounding_width_no_pad + pad_left + pad_right;
    bounding_height = bounding_height_no_pad + pad_top + pad_bottom;
}

void TextLayout::alignHorizontal(HALIGN halign, int box_width) {
    hori_align = halign;
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
        line.hori_align_offset = (halign_max_width - line.bounding_width) * align_mul;
        for (int j = line.begin; j < line.end; ++j) {
            auto& g = glyphs[j];
            g.glyph_rect.min.x += line.hori_align_offset;
            g.glyph_rect.max.x += line.hori_align_offset;
            g.x_midpoint += line.hori_align_offset;
        }
    }
    for (int i = 0; i < spans.size(); ++i) {
        auto& sp = spans[i];
        const auto& line = lines[sp.line_idx];
        sp.rc.min.x += line.hori_align_offset;
        sp.rc.max.x += line.hori_align_offset;
    }
}

void TextLayout::alignVertical(VALIGN valign, int box_height) {
    vert_align = valign;
    this->box_height = box_height;
    int offs = _calcVertOffset(valign, box_height);

    if (space == Y_UP) {
        offs = -offs;
    }
    for (int i = 0; i < lines.size(); ++i) {
        auto& line = lines[i];
        for (int j = line.begin; j < line.end; ++j) {
            auto& g = glyphs[j];
            g.glyph_rect.min.y += offs;
            g.glyph_rect.max.y += offs;
        }
    }
    for (int i = 0; i < spans.size(); ++i) {
        auto& sp = spans[i];
        sp.rc.min.y += offs;
        sp.rc.max.y += offs;
    }
}
void TextLayout::padHorizontal(int left, int right) {
    for (int i = 0; i < glyphs.size(); ++i) {
        auto& g = glyphs[i];
        g.glyph_rect.min.x += left;
        g.glyph_rect.max.x += left;
        g.x_midpoint += left;
    }
    for (int i = 0; i < spans.size(); ++i) {
        auto& sp = spans[i];
        sp.rc.min.x += left;
        sp.rc.max.x += left;
    }
    bounding_width += (left + right);
}
void TextLayout::padVertical(int top, int bottom) {
    for (int i = 0; i < glyphs.size(); ++i) {
        auto& g = glyphs[i];
        g.glyph_rect.min.y -= top;
        g.glyph_rect.max.y -= top;
    }
    for (int i = 0; i < spans.size(); ++i) {
        auto& sp = spans[i];
        sp.rc.min.y -= top;
        sp.rc.max.y -= top;
    }
    bounding_height += (top + bottom);
}

int TextLayout::hitTest(int x, int y) {
    auto ln = hitTestLine(x, y);
    if (!ln) {
        return 0;
    }
    
    int begin = ln->begin;
    int end = ln->end;
    while (begin != end) {
        int mid = begin + (end - begin) / 2;
        const auto& g = glyphs[mid];
        if (g.x_midpoint > x) {
            end = mid;
        } else {
            begin = mid + 1;
        }
    }
    if (begin >= ln->end) {
        return ln->raw_end;
    }
    return glyphs[begin].raw_idx;
}

TextLayout::Line* TextLayout::hitTestLine(int x, int y) {
    if (lines.empty()) {
        return nullptr;
    }
    int voffs = _calcVertOffset(vert_align, box_height);

    int line_idx = (y - descender - voffs) / line_height;
    line_idx = line_idx < 0 ? 0 : line_idx;
    line_idx = line_idx >= lines.size() ? lines.size() - 1 : line_idx;
    return &lines[line_idx];
}

