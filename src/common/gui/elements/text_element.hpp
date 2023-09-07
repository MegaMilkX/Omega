#pragma once

#include "element.hpp"
#include "gui/gui_text_buffer.hpp"


class GuiTextElement : public GuiElement {
    GuiTextElement* head_element = 0;
    std::vector<uint32_t> full_text_utf;
    int text_begin = 0;
    int text_end = 0;
    int select_begin = 0;
    int select_end = 0;
    bool is_selecting = false;
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
        : head_element(head), text_begin(text_begin), text_end(text_end) {
        setSize(gui::perc(100), gui::em(1));
        self_linear_size = text_end - text_begin;
    }
    
    int pickCursorPosition(const gfxm::vec2& mouse_local) {
        Font* font = getFont();
        float total_advance = 0;

        const std::vector<uint32_t>& text = head_element ? head_element->full_text_utf : full_text_utf;

        int cur = 0;
        int tab_offset = 0;
        for (int i = text_begin; i < text_end; ++i) {
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
        return cur;
    }
    void startSelection(int i) {
        select_begin = i;
        select_end = i;
        is_selecting = true;
        //guiCaptureMouse(this);
    }
    void stopSelection() {
        is_selecting = false;
        //guiCaptureMouse(0);
    }
    bool isSelecting() const {
        if (head_element) {
            return head_element->is_selecting;
        } else {
            return is_selecting;
        }
    }
    void setTextFromString(const std::string& text) {
        full_text_utf.resize(text.length());
        for (int i = 0; i < text.length(); ++i) {
            full_text_utf[i] = text[i];
        }
    }
public:
    GuiTextElement(const std::string& text = "") {
        setSize(gui::perc(100), gui::em(1));
        setTextFromString(text);
        text_begin = 0;
        text_end = text.length();
    }

    void setContent(const std::string& content) {
        setTextFromString(content);
        text_begin = 0;
        text_end = full_text_utf.size();
        self_linear_size = text_end - text_begin;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::LBUTTON_DOWN: {
            int i = pickCursorPosition(guiGetMousePos() - client_area.min);
            if (head_element) {
                head_element->startSelection(i);
            } else {
                startSelection(i);
            }
            return true;
        }
        case GUI_MSG::LBUTTON_UP: {
            if (head_element) {
                head_element->stopSelection();
            } else {
                stopSelection();
            }
            return true;
        }
        case GUI_MSG::MOUSE_MOVE: {
            if (isSelecting()) {
                int i = pickCursorPosition(guiGetMousePos() - client_area.min);
                if (head_element) {
                    head_element->select_end = i;
                } else {
                    select_end = i;
                }
                return true;
            } else {
                break;
            }
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
        int current_character_count = text_end - text_begin;

        const int available_width = client_area.max.x - client_area.min.x;
        int total_advance = 0;

        int hori_advance = 0;
        int glyphs_processed = 0;
        int tab_offset = 0;
        for (int i = text_begin; i < text_end; ++i) {
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

            if (available_width < total_advance) {
                if (glyphs_processed > 1) {
                    --glyphs_processed;
                }
                break;
            }
        }
        int new_end = text_begin + glyphs_processed;

        const int text_len = text_end - text_begin;
        if (glyphs_processed < text_len) {
            text_end = new_end;
            self_linear_size = text_end - text_begin;
            if (next_block) {
                next_block->text_begin = new_end;
                next_block->self_linear_size = next_block->text_end - next_block->text_begin;
            } else {
                next_block = new GuiTextElement(head_element ? head_element : this, new_end, text.size());
                next_block->setStyleClasses(getStyleClasses());
                next_block->self_linear_size = next_block->text_end - next_block->text_begin;
                getParent()->_insertAfter(this, next_block);
            }
        } else if(glyphs_processed == current_character_count) {
            while(next_block && available_width - total_advance > 0) {
                glyphs_processed = 0;
                assert(next_block->text_begin <= next_block->text_end);
                for (int i = next_block->text_begin; i < next_block->text_end; ++i) {
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

                    if (available_width < total_advance) {
                        break;
                    }
                    ++glyphs_processed;
                }
                if (glyphs_processed > 0) {
                    text_end = text_end + glyphs_processed;
                    self_linear_size = text_end - text_begin;
                    next_block->text_begin = text_end;
                    next_block->self_linear_size = next_block->text_end - next_block->text_begin;
                    if (next_block->text_begin == next_block->text_end) {
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

        int sel_begin = head_element ? head_element->select_begin : select_begin;
        int sel_end = head_element ? head_element->select_end : select_end;
        if (sel_begin > sel_end) {
            std::swap(sel_begin, sel_end);
        }

        int hori_advance = 0;
        int char_offset = 0;
        int line_offset = line_height - font->getDescender();
        for (int i = text_begin; i < text_end; ++i) {
            uint32_t ch = text[i];
            if (ch == 0x03) { // ETX - end of text
                break;
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
    }
};
