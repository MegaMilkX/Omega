#pragma once

#include "element.hpp"
#include "gui/gui_text_buffer.hpp"


extern void guiStartHightlight(int begin);
extern void guiUpdateHightlight(int end);
extern void guiStopHighlight();
extern void guiSetHighlight(int begin, int end);
extern bool guiIsHighlighting();
extern int guiGetHighlightBegin();
extern int guiGetHighlightEnd();
extern void guiSetTextCursor(int at, bool highlight);
extern int guiGetTextCursor();
extern void guiResetTextCursor();
extern uint32_t guiGetTextCursorTime();
extern void guiAdvanceTextCursor(int, bool highlight);

extern void        guiSetActiveWindow(GuiElement* elem);
extern GuiElement* guiGetActiveWindow();
extern void        guiSetFocusedWindow(GuiElement* elem);
extern void        guiUnfocusWindow(GuiElement* elem);
extern GuiElement* guiGetFocusedWindow();

extern int guiGetModifierKeysState();
extern bool guiIsModifierKeyPressed(int key);

extern gfxm::vec2 guiGetMousePos();

class GuiTextElement : public GuiElement {
    constexpr static float GLYPH_DIV = 64.f;

    struct TEXT_LINE {
        int begin;
        int end;
        int hori_advance_px;
    };
    std::vector<TEXT_LINE> text_lines;

    std::vector<uint32_t> full_text_utf;

    bool is_read_only = false;
    bool enable_word_wrap = true;
    bool use_elipsis = true; // TODO

    std::vector<float> vertices;
    std::vector<float> uv;
    std::vector<float> uv_lookup;
    std::vector<uint32_t> colors;
    std::vector<uint32_t> indices;
    // selection rectangles
    std::vector<float>      verts_selection;
    std::vector<uint32_t>   indices_selection;

    float getLineInnerAlignmentOffset(int ln) const {
        assert(ln >= 0 && ln < text_lines.size());

        const gfxm::rect& rc = getClientArea();

        float alignment_mul = .0f;
        switch (inner_alignment) {
        case HORIZONTAL_ALIGNMENT::CENTER: alignment_mul = .5f; break;
        case HORIZONTAL_ALIGNMENT::RIGHT: alignment_mul = 1.f; break;
        case HORIZONTAL_ALIGNMENT::LEFT: alignment_mul = 0.f; break;
        default: assert(false); break;
        }

        const float rc_width = rc.max.x - rc.min.x;

        return (rc_width - text_lines[ln].hori_advance_px) * alignment_mul;
    }
    int getVerticalAlignmentOffset() {
        auto font = getFont();
        int total_text_height = text_lines.size() * font->getLineHeight();
        int client_height = getClientHeight();
        int diff = client_height - total_text_height;
        
        float align_mul = .0f;
        switch (vertical_alignment) {
        case VERTICAL_ALIGNMENT::TOP: align_mul = .0f; break;
        case VERTICAL_ALIGNMENT::CENTER: align_mul = .5f; break;
        case VERTICAL_ALIGNMENT::BOTTOM: align_mul = 1.f; break;
        default: assert(false); break;
        }
        return diff * align_mul;
    }
    
    int pickCursorPosition(const gfxm::vec2& mouse_local) {
        Font* font = getFont();
        
        auto border_style = getStyleComponent<gui::style_border>();
        auto box_style = getStyleComponent<gui::style_box>();

        gui_rect gui_padding;
        if (box_style) {
            gui_padding = box_style->padding.value(gui_rect());
        }
        gui_rect gui_border_thickness;
        if (border_style) {
            gui_border_thickness = gui_rect(
                border_style->thickness_left.value(gui_float(0, gui_pixel)),
                border_style->thickness_top.value(gui_float(0, gui_pixel)),
                border_style->thickness_right.value(gui_float(0, gui_pixel)),
                border_style->thickness_bottom.value(gui_float(0, gui_pixel))
            );
        }

        // TODO: padding and border thickness should not support percent(?) and fill values
        gfxm::rect px_padding = gui_to_px(gui_padding, font, getClientSize());
        gfxm::rect px_border = gui_to_px(gui_border_thickness, font, getClientSize());

        gfxm::vec2 mouse = mouse_local;
        mouse -= px_padding.min;
        mouse -= px_border.min;
        mouse.y -= getVerticalAlignmentOffset();

        int cur = 0;
        int tab_offset = 0;

        int line_idx = gfxm::_max(0, gfxm::_min(int(text_lines.size()), int(mouse.y / font->getLineHeight())));
        float total_advance = getLineInnerAlignmentOffset(line_idx);
        int str_begin = text_lines[line_idx].begin;
        int str_end = text_lines[line_idx].end;
        const uint32_t* substr_begin = &full_text_utf[str_begin];
        const uint32_t* substr_end = &full_text_utf[str_end];

        const uint32_t* pchar = substr_begin;
        for (; pchar != substr_end; ++pchar) {
            uint32_t ch = *pchar;
            auto glyph = font->getGlyph(ch);
            int glyph_advance = 0;
            if (ch == '\t') {
                glyph_advance = font->getGlyph(' ').horiAdvance / GLYPH_DIV * (GUI_SPACES_PER_TAB - tab_offset);
            } else if (ch == '\n') {
                cur = pchar - substr_begin + str_begin;
                break;
            } else if (ch == 0x03) {
                cur = pchar - substr_begin + str_begin;
                break;
            } else {
                glyph_advance = glyph.horiAdvance / GLYPH_DIV;
                tab_offset = (tab_offset + 1) % GUI_SPACES_PER_TAB;
            }

            float mid = total_advance + glyph_advance * .5f;
            if (mid < mouse.x) {
                cur = pchar - substr_begin + str_begin + 1;
            } else {
                break;
            }

            total_advance += glyph_advance;
        }
        return linear_begin + cur;
    }
    void setTextFromString(const std::string& text) {
        full_text_utf.resize(text.length() + 1);
        for (int i = 0; i < text.length(); ++i) {
            full_text_utf[i] = text[i];
        }
        full_text_utf[full_text_utf.size() - 1] = 0x03;

        self_linear_size = full_text_utf.size();
    }

    void backspace() {
        int at = guiGetTextCursor() - linear_begin;
        if (at < 1) {
            return;
        }

        int begin = std::max(guiGetHighlightBegin(), linear_begin);
        int end = std::min(guiGetHighlightEnd(), linear_end - 1 /* -1 ETX */);
        if (begin < end) {
            full_text_utf.erase(full_text_utf.begin() + (begin - linear_begin), full_text_utf.begin() + (end - linear_begin));
            guiSetHighlight(0,0);
            guiSetTextCursor(begin, false);
            guiStopHighlight();
        } else {
            full_text_utf.erase(full_text_utf.begin() + (at - 1));
            guiAdvanceTextCursor(-1, false);
        }
    }
    void delete_() {
        int at = guiGetTextCursor() - linear_begin;

        int begin = std::max(guiGetHighlightBegin(), linear_begin);
        int end = std::min(guiGetHighlightEnd(), linear_end - 1 /* -1 ETX */);
        if (begin < end) {
            full_text_utf.erase(full_text_utf.begin() + (begin - linear_begin), full_text_utf.begin() + (end - linear_begin));
            guiSetHighlight(0,0);
            guiSetTextCursor(begin, false);
            guiStopHighlight();
        } else {
            if (at >= full_text_utf.size()) {
                return;
            }
            if (full_text_utf[at] == 0x03/* ETX */) {
                return;
            }
            full_text_utf.erase(full_text_utf.begin() + at);
        }
    }
    void putChar(uint32_t ch) {
        int begin = std::max(guiGetHighlightBegin(), linear_begin);
        int end = std::min(guiGetHighlightEnd(), linear_end - 1 /* -1 ETX */);
        if (begin < end) {
            full_text_utf.erase(full_text_utf.begin() + (begin - linear_begin), full_text_utf.begin() + (end - linear_begin));
            guiSetHighlight(0,0);
            guiSetTextCursor(begin, false);
            guiStopHighlight();
        }

        int at = guiGetTextCursor() - linear_begin;
        full_text_utf.insert(full_text_utf.begin() + at, ch);
        guiAdvanceTextCursor(1, false);
    }
    void newline() {
        int begin = std::max(guiGetHighlightBegin(), linear_begin);
        int end = std::min(guiGetHighlightEnd(), linear_end - 1 /* -1 ETX */);
        if (begin < end) {
            full_text_utf.erase(full_text_utf.begin() + (begin - linear_begin), full_text_utf.begin() + (end - linear_begin));
            guiSetHighlight(0,0);
            guiSetTextCursor(begin, false);
            guiStopHighlight();
        }

        int at = guiGetTextCursor() - linear_begin;
        full_text_utf.insert(full_text_utf.begin() + at, '\n');
        guiAdvanceTextCursor(1, false);
    }
    void advanceCursor(int offset, bool highlight = false) {
        /*
        int text_begin = getHead()->linear_begin;
        int text_end = text_begin + getHead()->full_text_utf.size();
        int to_begin = -(guiGetTextCursor() - text_begin);
        int to_end = text_end - guiGetTextCursor() - 1; // - 1 for ETX
        offset = std::max(to_begin, std::min(to_end, offset));
        */
        int new_cur = guiGetTextCursor() + offset;
        //guiSetFocusedWindow(e);
        new_cur = std::min(linear_end - 1 /* ETX */, std::max(linear_begin, new_cur));
        guiSetTextCursor(new_cur, highlight);

        //guiAdvanceTextCursor(offset, highlight);
    }
    void jumpLine(int offset, bool highlight = false) {
        int line_idx = 0;
        for (int i = 0; i < text_lines.size(); ++i) {
            if (guiGetTextCursor() - linear_begin < text_lines[i].begin) {
                break;
            }
            line_idx = i;
        }
        int inline_offset = guiGetTextCursor() - linear_begin - text_lines[line_idx].begin;
        line_idx = gfxm::_max(0, gfxm::_min(int(text_lines.size() - 1), line_idx + offset));
        if (inline_offset > text_lines[line_idx].end - text_lines[line_idx].begin) {
            inline_offset = text_lines[line_idx].end - text_lines[line_idx].begin - 1;
        }
        guiSetTextCursor(linear_begin + text_lines[line_idx].begin + inline_offset, highlight);

        // TODO:
        /*if (offset < 0) {
            GuiTextElement* e = this;
            while (e->prev_line && offset) {
                e = e->prev_line;
                ++offset;
            }
            int lcl_cur = guiGetTextCursor() - linear_begin;
            int at = e->linear_begin + std::min(lcl_cur, e->linear_end - e->linear_begin - 1);
            guiSetFocusedWindow(e);
            guiSetTextCursor(at, highlight);
        } else if (offset > 0) {
            GuiTextElement* e = this;
            while (e->next_line && offset) {
                e = e->next_line.get();
                --offset;
            }
            int lcl_cur = guiGetTextCursor() - linear_begin;
            int at = e->linear_begin + std::min(lcl_cur, e->linear_end - e->linear_begin - 1);
            guiSetFocusedWindow(e);
            guiSetTextCursor(at, highlight);
        }*/
    }
public:
    enum class HORIZONTAL_ALIGNMENT {
        LEFT,
        CENTER,
        RIGHT
    };
    enum class VERTICAL_ALIGNMENT {
        TOP,
        CENTER,
        BOTTOM
    };

protected:
    HORIZONTAL_ALIGNMENT inner_alignment = HORIZONTAL_ALIGNMENT::LEFT;
    VERTICAL_ALIGNMENT vertical_alignment = VERTICAL_ALIGNMENT::CENTER;

public:
    GuiTextElement(const std::string& text = "") {
        setSize(gui::fill(), gui::content());
        setTextFromString(text);
    }

    // TODO: Should probably be a GUI_FLAG_READ_ONLY
    void setReadOnly(bool val) {
        is_read_only = val;
    }
    void setContent(const std::string& content) {
        setTextFromString(content);
    }
    void setInnerAlignment(HORIZONTAL_ALIGNMENT halign, VERTICAL_ALIGNMENT valign = VERTICAL_ALIGNMENT::TOP) {
        inner_alignment = halign;
        vertical_alignment = valign;
    }

    std::string getText() const {
        int len = full_text_utf.size() - 1; // -1 for ETX
        if (len == -1) {
            return std::string();
        }
        std::string out;
        out.resize(len);
        for (int i = 0; i < len; ++i) {
            out[i] = (char)full_text_utf[i];
        }
        return out;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::UNICHAR: {
            switch (params.getA<GUI_CHAR>()) {
            case GUI_CHAR::BACKSPACE: {
                backspace();
                return true;
            }
            case GUI_CHAR::RETURN: {
                newline();
                return true;
            }
            default: {
                uint32_t ch = (uint32_t)params.getA<GUI_CHAR>();
                LOG_DBG(std::format("GUI_MSG::UNICHAR: {:#02x}", ch));
                if (ch > 0x1F || ch == 0x0A) {
                    putChar(ch);
                }
                return true;
            }
            }
            break;
        }
        case GUI_MSG::KEYDOWN: {
            switch (params.getA<uint16_t>()) {
            case VK_DELETE: {
                delete_();
                return true;
            }
            case VK_LEFT: {
                advanceCursor(-1, guiIsModifierKeyPressed(GUI_KEY_SHIFT));
                return true;
            }
            case VK_RIGHT: {
                advanceCursor(1, guiIsModifierKeyPressed(GUI_KEY_SHIFT));
                return true;
            }
            case VK_UP: {
                jumpLine(-1, guiIsModifierKeyPressed(GUI_KEY_SHIFT));
                return true;
            }
            case VK_DOWN: {
                jumpLine(1, guiIsModifierKeyPressed(GUI_KEY_SHIFT));
                return true;
            }
            }
            }
            break;
        case GUI_MSG::FOCUS:
            if(!is_read_only) {
                return true;
            } else {
                return false;
            }
        case GUI_MSG::UNFOCUS: {
            //auto new_focused_line = dynamic_cast<GuiTextElement*>(params.getA<GuiElement*>());
            
            //if (!new_focused_line || new_focused_line->getHead() != getHead()) {
                guiResetTextCursor();
                guiSetHighlight(0,0);
            //}

            /*
            if (guiGetTextCursor() >= linear_begin && guiGetTextCursor() < linear_end) {
                //guiResetTextCursor();
            }
            if (getHead()->linear_begin <= guiGetHighlightEnd() && getTail()->linear_end >= guiGetHighlightBegin()) {
                //guiSetHighlight(0,0);
            }*/
            return true;
        }
        case GUI_MSG::LBUTTON_DOWN: {
            if(!is_read_only) {
                int i = pickCursorPosition(guiGetMousePos() - getGlobalPosition());
                guiStartHightlight(i);
            }
            return true;
        }
        case GUI_MSG::TEXT_HIGHTLIGHT_UPDATE: {
            if(!is_read_only) {
                int i = pickCursorPosition(guiGetMousePos() - getGlobalPosition());
                guiUpdateHightlight(i);
                guiSetFocusedWindow(this);
            }
            return true;
        }
        }
        return GuiElement::onMessage(msg, params);
    }

    void onLayout(const gfxm::vec2& extents, uint64_t flags) {
        Font* font = getFont();
        auto box_style = getStyleComponent<gui::style_box>();
        gui_rect gui_padding;
        gfxm::rect px_padding;
        if (box_style) {
            gui_padding = box_style->padding.has_value() ? box_style->padding.value() : gui_rect();
        }
        px_padding = gui_to_px(gui_padding, font, getClientSize());

        if (flags & GUI_LAYOUT_WIDTH_PASS) {
            rc_bounds.min.x = 0;
            rc_bounds.max.x = roundf(extents.x);
            client_area.min.x = rc_bounds.min.x;
            client_area.max.x = rc_bounds.max.x;
            client_area.min.x += px_padding.min.x;
            client_area.max.x -= px_padding.max.x;

            text_lines.clear();

            const int available_width = client_area.max.x - client_area.min.x;
            int str_begin = 0;
            int str_end = full_text_utf.size();
            int max_hori_advance_px = 0;
            while(str_begin != str_end) {
                int total_advance = 0;
                int tab_offset = 0;
                int ichar = str_begin;
                int word_wrap_end = str_begin;
                int word_wrap_total_advance = total_advance;
                uint32_t lastchar = ' ';
                TEXT_LINE line = { 0 };
            
                for (; ichar != str_end; ++ichar) {
                    uint32_t ch = full_text_utf[ichar];

                    auto glyph = font->getGlyph(ch);
                    int glyph_advance = 0;
                    if (ch == '\t') {
                        glyph_advance = font->getGlyph(' ').horiAdvance / GLYPH_DIV * (GUI_SPACES_PER_TAB - tab_offset);
                    } else if (ch == 0x03 /* ETX */) {
                        glyph_advance = 0;
                    } else {
                        glyph_advance = glyph.horiAdvance / GLYPH_DIV;
                        tab_offset = (tab_offset + 1) % GUI_SPACES_PER_TAB;
                    }

                    if (!isspace(ch) && isspace(lastchar)) {
                        word_wrap_end = ichar;
                        word_wrap_total_advance = total_advance;
                    }
                    lastchar = ch;

                    if (available_width < total_advance + glyph_advance
                        && ichar > str_begin // Must put at least one char even if it doesn't fit
                        && (flags & GUI_LAYOUT_FIT_CONTENT) == 0
                        && enable_word_wrap
                    ) {
                        break;
                    }
                    total_advance += glyph_advance;

                    if (ch == '\n' || ch == 0x03 /* ETX */) {
                        ++ichar;
                        word_wrap_end = ichar;
                        word_wrap_total_advance = total_advance;
                        break;
                    }
                }

                if(word_wrap_end == str_begin) {
                    line.hori_advance_px = total_advance;
                } else {
                    line.hori_advance_px = word_wrap_total_advance;
                    ichar = word_wrap_end;
                }
                max_hori_advance_px = gfxm::_max(max_hori_advance_px, line.hori_advance_px);

                line.begin = str_begin;
                line.end = ichar;
                text_lines.push_back(line);
                
                // Can't wrap the first character on line
                assert(str_begin != ichar);

                str_begin = ichar;
            }

            if (flags & GUI_LAYOUT_FIT_CONTENT) {
                client_area.max.x = client_area.min.x + max_hori_advance_px;
                rc_bounds.max.x = rc_bounds.min.x + max_hori_advance_px + px_padding.min.x + px_padding.max.x;
            }

            self_linear_size = full_text_utf.size();
            linear_end = linear_begin + self_linear_size;
        }

        if (flags & GUI_LAYOUT_HEIGHT_PASS) {
            rc_bounds.min.y = 0;
            rc_bounds.max.y = roundf(extents.y);
            client_area.min.y = rc_bounds.min.y;
            client_area.max.y = rc_bounds.max.y;
            client_area.min.y += px_padding.min.y;
            client_area.max.y -= px_padding.max.y;

            if (flags & GUI_LAYOUT_FIT_CONTENT) {
                const int line_height = font->getLineHeight();
                //int line_offset = line_height - font->getDescender();// + vert_alignment_offset;
                client_area.max.y = client_area.min.y + text_lines.size() * line_height;
                rc_bounds.max.y = rc_bounds.min.y + text_lines.size() * line_height + px_padding.min.y + px_padding.max.y;
            }
        }

        if (flags & GUI_LAYOUT_POSITION_PASS) {

        }
    }

    void onDraw() override {
        GuiElement::onDraw();

        Font* font = getFont();
        uint32_t color = GUI_COL_WHITE;
        uint32_t color_highlighted = GUI_COL_TEXT_HIGHLIGHTED;
        uint32_t bg_color = GUI_COL_TEXT;
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

        int elipsis_width = 0;
        {
            auto g = font->getGlyph('.');
            elipsis_width = g.horiAdvance / GLYPH_DIV * 3;
        }

        int correction = linear_begin;
        int sel_begin = guiGetHighlightBegin() - correction;
        int sel_end = guiGetHighlightEnd() - correction;
        int px_cursor_at_x = -1;
        int px_cursor_at_y = -1;

        int vert_alignment_offset = getVerticalAlignmentOffset();
        int line_offset = line_height - font->getDescender() + vert_alignment_offset;
        for (int i = 0; i < text_lines.size(); ++i) {
            const TEXT_LINE& ln = text_lines[i];

            int hori_advance = getLineInnerAlignmentOffset(i);
            int char_offset = 0;
            for(int ichar = ln.begin; ichar != ln.end; ++ichar) {
                if (guiGetTextCursor() == linear_begin + ichar) {
                    px_cursor_at_x = hori_advance;
                    px_cursor_at_y = line_offset;
                }

                uint32_t ch = full_text_utf[ichar];
                if (ch == 0x03) { // ETX - end of text
                    break;
                }

                auto glyph = font->getGlyph(ch);
                int y_offset = glyph.height - glyph.bearingY;
                int x_offset = glyph.bearingX;
                int glyph_advance = 0;
                if (ch == '\t') {
                    int rem = char_offset % GUI_SPACES_PER_TAB;
                    glyph_advance = font->getGlyph(' ').horiAdvance / GLYPH_DIV * (GUI_SPACES_PER_TAB - rem);
                } else {
                    glyph_advance = glyph.horiAdvance / GLYPH_DIV;
                    ++char_offset;
                }

                // Selection quads            
                if(ichar >= sel_begin && ichar < sel_end) {
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
                    hori_advance + x_offset - 1.f,               0 - y_offset - line_offset - 1.f,            0,
                    hori_advance + glyph.width + x_offset + 1.f,     0 - y_offset - line_offset - 1.f,            0,
                    hori_advance + x_offset - 1.f,               glyph.height - y_offset - line_offset + 1.f,     0,
                    hori_advance + glyph.width + x_offset + 1.f,     glyph.height - y_offset - line_offset + 1.f,     0
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

                if (linear_begin + ichar >= guiGetHighlightBegin()
                    && linear_begin + ichar < guiGetHighlightEnd()) {
                    colors.insert(colors.end(), 4, color_highlighted);
                } else {
                    colors.insert(colors.end(), 4, color);
                }

                hori_advance += glyph.horiAdvance / GLYPH_DIV;
            }
            line_offset += line_height;
        }
        // selection quads
        if (verts_selection.size() > 0) {
            guiDrawTextHighlight(
                (gfxm::vec3*)verts_selection.data(),
                verts_selection.size() / 3,
                indices_selection.data(), indices_selection.size(),
                bg_color
            ).model_transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(client_area.min.x, client_area.min.y, .0f));
        }
        if (vertices.size() > 0) {
            const float x_at = roundf(client_area.min.x);
            const float y_at = roundf(client_area.min.y);

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
            ).model_transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(x_at, y_at, .0f));
        }
        if (px_cursor_at_x >= 0) {
            gfxm::rect rc_rect(
                gfxm::vec2(client_area.min.x + px_cursor_at_x, client_area.min.y + px_cursor_at_y - line_height + font->getDescender()),
                gfxm::vec2(client_area.min.x + px_cursor_at_x, client_area.min.y + px_cursor_at_y + font->getDescender()) + gfxm::vec2(2, 0)
            );
            if ((time(0) - guiGetTextCursorTime()) % 2 == 0) {
                guiDrawRect(rc_rect, color);
            }
        }
    }
    /*
    void onDraw_old() {
        GuiElement::onDraw();

        Font* font = getFont();
        gfxm::rect rc = getClientArea();
        uint32_t color = GUI_COL_WHITE;
        uint32_t color_highlighted = GUI_COL_TEXT_HIGHLIGHTED;
        uint32_t bg_color = GUI_COL_TEXT;
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

        const std::vector<uint32_t>& text = full_text_utf;// head_element ? head_element->full_text_utf : full_text_utf;
        int correction = linear_begin;// head_element ? head_element->linear_begin : linear_begin;
        int sel_begin = guiGetHighlightBegin() - correction;
        int sel_end = guiGetHighlightEnd() - correction;

        int px_cursor_at = -1;

        const int vert_alignment_offset = getClientHeight() * .5f - (line_height - font->getDescender()) * .5f;

        int hori_advance = getInnerAlignmentOffset();
        int char_offset = 0;
        int line_offset = line_height - font->getDescender() + vert_alignment_offset;
        for(int ichar = str_begin; ichar != str_end; ++ichar) {
            if (guiGetTextCursor() == linear_begin + ichar - str_begin) {
                px_cursor_at = hori_advance;
            }

            uint32_t ch = getHead()->full_text_utf[ichar];
            if (ch == 0x03) { // ETX - end of text
                break;
            }           

            auto glyph = font->getGlyph(ch);
            int y_offset = glyph.height - glyph.bearingY;
            int x_offset = glyph.bearingX;
            int glyph_advance = 0;
            if (ch == '\t') {
                int rem = char_offset % GUI_SPACES_PER_TAB;
                glyph_advance = font->getGlyph(' ').horiAdvance / GLYPH_DIV * (GUI_SPACES_PER_TAB - rem);
            } else {
                glyph_advance = glyph.horiAdvance / GLYPH_DIV;
                ++char_offset;
            }

            // Selection quads            
            if(ichar - str_begin >= sel_begin && ichar - str_begin < sel_end) {
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
                hori_advance + x_offset - 1.f,               0 - y_offset - line_offset - 1.f,            0,
                hori_advance + glyph.width + x_offset + 1.f,     0 - y_offset - line_offset - 1.f,            0,
                hori_advance + x_offset - 1.f,               glyph.height - y_offset - line_offset + 1.f,     0,
                hori_advance + glyph.width + x_offset + 1.f,     glyph.height - y_offset - line_offset + 1.f,     0
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

            if (linear_begin + ichar - str_begin >= guiGetHighlightBegin()
                && linear_begin + ichar - str_begin < guiGetHighlightEnd()) {
                colors.insert(colors.end(), 4, color_highlighted);
            } else {
                colors.insert(colors.end(), 4, color);
            }

            hori_advance += glyph.horiAdvance / GLYPH_DIV;
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
            const float x_at = roundf(rc.min.x);
            const float y_at = roundf(rc.min.y);

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
            ).model_transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(x_at, y_at, .0f));
        }
        if (px_cursor_at >= 0) {
            const int vert_alignment_offset = getClientHeight() * .5f - (line_height - font->getDescender()) * .5f;
            const int line_offset = line_height - font->getDescender() + vert_alignment_offset;
            gfxm::rect rc_rect(
                gfxm::vec2(client_area.min.x + px_cursor_at, client_area.min.y + line_offset - line_height + font->getDescender()),
                gfxm::vec2(client_area.min.x + px_cursor_at, client_area.min.y + line_offset + font->getDescender()) + gfxm::vec2(2, 0)
            );
            if ((time(0) - guiGetTextCursorTime()) % 2 == 0) {
                guiDrawRect(rc_rect, color);
            }
        }
    }*/
};
