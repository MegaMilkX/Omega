#pragma once

#include "element.hpp"
#include "gui/gui_text_buffer.hpp"

extern void guiStartHightlight(int at);
extern void guiUpdateHightlight(int end);
extern void guiStopHighlight();
extern int guiGetHighlightBegin();
extern int guiGetHighlightEnd();
extern int guiGetTextCursor();
extern void guiResetTextCursor();
extern void guiAdvanceTextCursor(int);

class GuiTextElement : public GuiElement {
    GuiTextElement* head_element = 0;
    std::vector<uint32_t> full_text_utf;
    GuiTextElement* next_block = 0;

    std::vector<float> vertices;
    std::vector<float> uv;
    std::vector<float> uv_lookup;
    std::vector<uint32_t> colors;
    std::vector<uint32_t> indices;
    // selection rectangles
    std::vector<float>      verts_selection;
    std::vector<uint32_t>   indices_selection;

    GuiTextElement(GuiTextElement* head, int text_begin, int text_end)
        : head_element(head) {
        setSize(gui::perc(100), gui::em(1));
        linear_begin = text_begin;
        linear_end = text_end;
        self_linear_size = text_end - text_begin;
    }
    
    int pickCursorPosition(const gfxm::vec2& mouse_local) {
        Font* font = getFont();
        float total_advance = 0;

        const std::vector<uint32_t>& text = head_element ? head_element->full_text_utf : full_text_utf;
        int correction = head_element ? head_element->linear_begin : linear_begin;
        int text_begin_ = linear_begin - correction;
        int text_end_ = linear_end - correction;

        int cur = 0;
        int tab_offset = 0;
        for (int i = text_begin_; i < text_end_; ++i) {
            uint32_t ch = text[i];
            auto glyph = font->getGlyph(ch);
            int glyph_advance = 0;
            if (ch == '\t') {
                glyph_advance = font->getGlyph(' ').horiAdvance / 64 * (GUI_SPACES_PER_TAB - tab_offset);
            } else {
                glyph_advance = glyph.horiAdvance / 64;
                tab_offset = (tab_offset + 1) % GUI_SPACES_PER_TAB;
            }
            float mid = total_advance - glyph_advance * .5f;
            if (mid < mouse_local.x) {
                cur = i;
            } else {
                break;
            }

            total_advance += glyph_advance;
        }
        return correction + cur;
    }
    void setTextFromString(const std::string& text) {
        full_text_utf.resize(text.length());
        for (int i = 0; i < text.length(); ++i) {
            full_text_utf[i] = text[i];
        }
    }
    void adjustRangeFromThis(int i) {
        self_linear_size += i;
        linear_end += i;
        if (next_block) {
            next_block->linear_begin = linear_end;
            next_block->self_linear_size = next_block->linear_end - next_block->linear_begin;
            next_block->adjustEnd(i);
        }
    }
    void adjustEnd(int i) {
        if (next_block) {
            next_block->adjustEnd(i);
        } else {
            linear_end += i;
            self_linear_size = linear_end - linear_begin;
        }
    }
public:
    GuiTextElement(const std::string& text = "") {
        setSize(gui::perc(100), gui::em(1));
        setTextFromString(text);
        linear_begin = 0;
        linear_end = text.length();
    }

    void setContent(const std::string& content) {
        setTextFromString(content);
        linear_begin = 0;
        linear_end = full_text_utf.size();
        self_linear_size = linear_end - linear_begin;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::UNICHAR: {
            switch (params.getA<GUI_CHAR>()) {
            case GUI_CHAR::BACKSPACE: {
                int correction = head_element ? head_element->linear_begin : linear_begin;
                int inner_cursor = guiGetTextCursor() - correction;
                int local_inner_cursor = guiGetTextCursor() - linear_begin;
                if (inner_cursor == 0) {
                    break;
                }
                if (head_element) {
                    head_element->full_text_utf.erase(head_element->full_text_utf.begin() + inner_cursor - 1);
                } else {
                    full_text_utf.erase(full_text_utf.begin() + inner_cursor - 1);
                }
                guiAdvanceTextCursor(-1);
                adjustRangeFromThis(-1);
                break;
            }
            case GUI_CHAR::ESCAPE: {
                guiSetFocusedWindow(0);
                break;
            }
            case GUI_CHAR::RETURN: {
                int correction = head_element ? head_element->linear_begin : linear_begin;
                int inner_cursor = guiGetTextCursor() - correction;
                if (head_element) {
                    head_element->full_text_utf.insert(head_element->full_text_utf.begin() + inner_cursor, '\n');
                } else {
                    full_text_utf.insert(full_text_utf.begin() + inner_cursor, '\n');
                }
                guiAdvanceTextCursor(1);
                adjustRangeFromThis(1);
                break;
            }
            default: {
                int correction = head_element ? head_element->linear_begin : linear_begin;
                int inner_cursor = guiGetTextCursor() - correction;
                uint32_t ch = (uint32_t)params.getA<GUI_CHAR>();
                if (ch > 0x1F || ch == 0x0A) {
                    if (head_element) {
                        head_element->full_text_utf.insert(head_element->full_text_utf.begin() + inner_cursor, ch);
                    } else {
                        full_text_utf.insert(full_text_utf.begin() + inner_cursor, ch);
                    }
                    guiAdvanceTextCursor(1);
                    adjustRangeFromThis(1);
                }
            }
            }
            return true;
        }
        case GUI_MSG::FOCUS:
            return true;
        case GUI_MSG::UNFOCUS:
            if (guiGetTextCursor() >= linear_begin && guiGetTextCursor() < linear_end) {
                guiResetTextCursor();
            }
            return true;
        case GUI_MSG::LBUTTON_DOWN: {
            int i = pickCursorPosition(guiGetMousePos() - client_area.min);
            guiStartHightlight(i);
            return true;
        }
        case GUI_MSG::TEXT_HIGHTLIGHT_UPDATE: {
            int i = pickCursorPosition(guiGetMousePos() - client_area.min);
            guiUpdateHightlight(i);
            guiSetFocusedWindow(this);
            return true;
        }
        }
        return GuiElement::onMessage(msg, params);
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        rc_bounds.min.x = roundf(rc_bounds.min.x);
        rc_bounds.min.y = roundf(rc_bounds.min.y);
        rc_bounds.max.x = roundf(rc_bounds.max.x);
        rc_bounds.max.y = roundf(rc_bounds.max.y);
        client_area = rc_bounds;        

        Font* font = getFont();

        const std::vector<uint32_t>& text = head_element ? head_element->full_text_utf : full_text_utf;
        int current_character_count = linear_end - linear_begin;
        int correction = head_element ? head_element->linear_begin : linear_begin;
        int text_begin_ = linear_begin - correction;
        int text_end_ = linear_end - correction;

        const int available_width = client_area.max.x - client_area.min.x;
        int total_advance = 0;

        bool line_break = false;
        int hori_advance = 0;
        int glyphs_processed = 0;
        int tab_offset = 0;
        for (int i = text_begin_; i < text_end_; ++i) {
            uint32_t ch = text[i];
            /*
            if (ch == 0x03) { // ETX - end of text
                break;
            }*/
            auto glyph = font->getGlyph(ch);
            int glyph_advance = 0;
            if (ch == '\t') {
                glyph_advance = font->getGlyph(' ').horiAdvance / 64 * (GUI_SPACES_PER_TAB - tab_offset);
            } else {
                glyph_advance = glyph.horiAdvance / 64;
                tab_offset = (tab_offset + 1) % GUI_SPACES_PER_TAB;
            }/*
            if (isspace(ch)) {
                hori_advance += glyph_advance;
                ++glyphs_processed;
                continue;
            }*/

            total_advance += glyph_advance;
            ++glyphs_processed;

            if (available_width < total_advance && !isspace(ch)) {
                if (glyphs_processed > 1) {
                    --glyphs_processed;
                }
                break;
            }
            if (ch == '\n') {
                line_break = true;
                break;
            }
        }
        int new_end = linear_begin + glyphs_processed;

        const int text_len = text_end_ - text_begin_;
        if (glyphs_processed < text_len) {
            linear_end = new_end;
            self_linear_size = linear_end - linear_begin;
            if (next_block) {
                next_block->linear_begin = new_end;
                next_block->self_linear_size = next_block->linear_end - next_block->linear_begin;
            } else {
                next_block = new GuiTextElement(head_element ? head_element : this, new_end, correction + text.size());
                next_block->setStyleClasses(getStyleClasses());
                next_block->self_linear_size = next_block->linear_end - next_block->linear_begin;
                getParent()->_insertAfter(this, next_block);
            }
        } else if(glyphs_processed == current_character_count) {
            while(next_block && available_width - total_advance > 0 && !line_break) {
                glyphs_processed = 0;
                assert(next_block->text_begin <= next_block->text_end);
                int next_text_begin_ = next_block->linear_begin - correction;
                int next_text_end_ = next_block->linear_end - correction;
                for (int i = next_text_begin_; i < next_text_end_; ++i) {
                    uint32_t ch = text[i];
                    auto glyph = font->getGlyph(ch);
                    int glyph_advance = 0;
                    if (ch == '\t') {
                        glyph_advance = font->getGlyph(' ').horiAdvance / 64 * (GUI_SPACES_PER_TAB - tab_offset);
                    } else {
                        glyph_advance = glyph.horiAdvance / 64;
                        tab_offset = (tab_offset + 1) % GUI_SPACES_PER_TAB;
                    }

                    total_advance += glyph_advance;

                    if (available_width < total_advance && !isspace(ch)) {
                        break;
                    }
                    if (ch == '\n') {
                        line_break = true;
                        ++glyphs_processed;
                        break;
                    }
                    ++glyphs_processed;
                }
                if (glyphs_processed > 0) {
                    linear_end = linear_end + glyphs_processed;
                    self_linear_size = linear_end - linear_begin;
                    next_block->linear_begin = linear_end;
                    next_block->self_linear_size = next_block->linear_end - next_block->linear_begin;
                    if (next_block->linear_begin == next_block->linear_end) {
                        auto block_to_remove = next_block;
                        next_block = next_block->next_block;
                        getParent()->_erase(block_to_remove);
                        delete block_to_remove;
                    }
                }
            }
        }
        
        

        //GuiElement::onLayout(rc, flags);
    }
    void onDraw() override {
        GuiElement::onDraw();

        Font* font = getFont();
        gfxm::rect rc = getBoundingRect();
        uint32_t color = GUI_COL_WHITE;
        uint32_t bg_color = GUI_COL_BLUE;
        auto color_style = getStyleComponent<gui::style_color>();
        auto bg_color_style = getStyleComponent<gui::style_background_color>();
        if (color_style && color_style->color.has_value()) {
            color = color_style->color.value();
        }
        if (bg_color_style && bg_color_style->color.has_value()) {
            //bg_color = bg_color_style->color.value();
        }


        const int line_height = font->getLineHeight();

        vertices.clear();
        uv.clear();
        uv_lookup.clear();
        colors.clear();
        indices.clear();

        verts_selection.clear();
        indices_selection.clear();

        const std::vector<uint32_t>& text = head_element ? head_element->full_text_utf : full_text_utf;
        int correction = head_element ? head_element->linear_begin : linear_begin;
        int sel_begin = guiGetHighlightBegin() - correction;
        int sel_end = guiGetHighlightEnd() - correction;

        int text_begin_ = linear_begin - correction;
        int text_end_ = linear_end - correction;

        int px_cursor_at = -1;

        int hori_advance = 0;
        int char_offset = 0;
        int line_offset = line_height - font->getDescender();
        for (int i = text_begin_; i < text_end_; ++i) {
            uint32_t ch = text[i];
            if (ch == 0x03) { // ETX - end of text
                break;
            }

            if (guiGetTextCursor() == i + correction) {
                px_cursor_at = hori_advance;
            }

            auto glyph = font->getGlyph(ch);
            int y_offset = glyph.height - glyph.bearingY;
            int x_offset = glyph.bearingX;
            int glyph_advance = 0;
            if (ch == '\t') {
                int rem = char_offset % GUI_SPACES_PER_TAB;
                glyph_advance = font->getGlyph(' ').horiAdvance / 64 * (GUI_SPACES_PER_TAB - rem);
            } else {
                glyph_advance = glyph.horiAdvance / 64;
                ++char_offset;
            }

            // Selection quads
            if(i >= sel_begin && i < sel_end) {
                uint32_t base_index = verts_selection.size() / 3;
                float sel_verts[] = {
                    hori_advance,                       0 - line_offset - font->getDescender(),            0,
                    hori_advance + glyph_advance,       0 - line_offset - font->getDescender(),            0,
                    hori_advance,                       line_height - line_offset - font->getDescender(),     0,
                    hori_advance + glyph_advance,       line_height - line_offset - font->getDescender(),     0
                };
                verts_selection.insert(verts_selection.end(), sel_verts, sel_verts + sizeof(sel_verts) / sizeof(sel_verts[0]));
                uint32_t sel_indices[] = {
                    base_index, base_index + 1, base_index + 2,
                    base_index + 1, base_index + 3, base_index + 2
                };
                indices_selection.insert(indices_selection.end(), sel_indices, sel_indices + sizeof(sel_indices) / sizeof(sel_indices[0]));
            }

            if (isspace(ch)) {
                hori_advance += glyph_advance;
                continue;
            }

            uint32_t base_index = vertices.size() / 3;
            float glyph_verts[] = {
                hori_advance + x_offset,               0 - y_offset - line_offset,            0,
                hori_advance + glyph.width + x_offset,     0 - y_offset - line_offset,            0,
                hori_advance + x_offset,               glyph.height - y_offset - line_offset,     0,
                hori_advance + glyph.width + x_offset,     glyph.height - y_offset - line_offset,     0
            };
            vertices.insert(vertices.end(), glyph_verts, glyph_verts + sizeof(glyph_verts) / sizeof(glyph_verts[0]));
            
            uint32_t glyph_indices[] = {
                base_index, base_index + 1, base_index + 2,
                base_index + 1, base_index + 3, base_index + 2
            };
            indices.insert(indices.end(), glyph_indices, glyph_indices + sizeof(glyph_indices) / sizeof(glyph_indices[0]));

            float glyph_uv[] = { // Flipped
                0.0f, 1.0f,     1.0f, 1.0f,     0.0f, 0.0f,     1.0f, 0.0f
            };
            uv.insert(uv.end(), glyph_uv, glyph_uv + sizeof(glyph_uv) / sizeof(glyph_uv[0]));

            float glyph_uv_lookup[] = {
                ch * 4, ch * 4 + 1, ch * 4 + 3, ch * 4 + 2
            };
            uv_lookup.insert(uv_lookup.end(), glyph_uv_lookup, glyph_uv_lookup + sizeof(glyph_uv_lookup) / sizeof(glyph_uv_lookup[0]));

            colors.insert(colors.end(), 4, color);

            hori_advance += glyph.horiAdvance / 64;
        }

        // selection quads
        if (verts_selection.size() > 0) {
            guiDrawTextHighlight(
                (gfxm::vec3*)verts_selection.data(),
                verts_selection.size() / 3,
                indices_selection.data(), indices_selection.size(),
                bg_color
            ).model_transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(rc.min.x, rc.min.y, .0f));
        }
        if (vertices.size() > 0) {
            _guiDrawText(
                (gfxm::vec3*)vertices.data(),
                (gfxm::vec2*)uv.data(),
                colors.data(),
                uv_lookup.data(),
                vertices.size() / 3,
                indices.data(), indices.size(),
                color,
                font->getTextureData()->atlas->getId(),
                font->getTextureData()->lut->getId(),
                font->getTextureData()->lut->getWidth()
            ).model_transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(rc.min.x, rc.min.y, .0f));
        }
        if (px_cursor_at >= 0) {
            gfxm::rect rc_line(
                gfxm::vec2(client_area.min.x + px_cursor_at, client_area.min.y),
                gfxm::vec2(client_area.min.x + px_cursor_at, client_area.max.y)
            );
            if (time(0) % 2 == 0) {
                guiDrawLine(rc_line, 1.f, color);
            }
        }
    }
};
