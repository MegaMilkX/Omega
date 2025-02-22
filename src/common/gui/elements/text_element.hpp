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
extern uint32_t guiGetTextCursorTime();
extern void guiAdvanceTextCursor(int);


class GuiTextElement : public GuiElement {
    constexpr static float GLYPH_DIV = 64.f;

    GuiTextElement* head = 0;

    std::vector<uint32_t> full_text_utf;

    //uint32_t* substr_begin = 0;
    //uint32_t* substr_end = 0;
    int str_begin = 0;
    int str_end = 0;
    int depth = 0;
    
    int hori_advance_px = 0;

    std::unique_ptr<GuiTextElement> next_line;

    std::vector<float> vertices;
    std::vector<float> uv;
    std::vector<float> uv_lookup;
    std::vector<uint32_t> colors;
    std::vector<uint32_t> indices;
    // selection rectangles
    std::vector<float>      verts_selection;
    std::vector<uint32_t>   indices_selection;

    GuiTextElement(GuiTextElement* head, int substr_begin, int substr_end, int depth)
    : head(head), str_begin(substr_begin), str_end(substr_end), depth(depth) {
        setSize(gui::perc(100), gui::em(1));
        linear_begin = 0;
        linear_end = substr_end - substr_begin;
    }

    bool isHead() const {
        return depth == 0;
    }
    GuiTextElement* getHead() {
        if (!head) {
            return this;
        }
        return head;
    }
    const GuiTextElement* getHead() const {
        if (!head) {
            return this;
        }
        return head;
    }

    float getInnerAlignmentOffset() const {
        const gfxm::rect& rc = getBoundingRect();

        float alignment_mul = .0f;
        switch (inner_alignment) {
        case TEXT_ALIGNMENT::CENTER: alignment_mul = .5f; break;
        case TEXT_ALIGNMENT::RIGHT: alignment_mul = 1.f; break;
        default: assert(false); break;
        }
        
        const float rc_width = rc.max.x - rc.min.x;
        
        return (rc_width - hori_advance_px) * alignment_mul;
    }
    
    int pickCursorPosition(const gfxm::vec2& mouse_local) {
        Font* font = getFont();
        float total_advance = getInnerAlignmentOffset();

        int cur = 0;
        int tab_offset = 0;

        auto head = getHead();
        const uint32_t* substr_begin = &head->full_text_utf[str_begin];
        const uint32_t* substr_end = substr_begin + str_end;

        const uint32_t* pchar = substr_begin;
        for (; pchar != substr_end; ++pchar) {
            uint32_t ch = *pchar;
            auto glyph = font->getGlyph(ch);
            int glyph_advance = 0;
            if (ch == '\t') {
                glyph_advance = font->getGlyph(' ').horiAdvance / GLYPH_DIV * (GUI_SPACES_PER_TAB - tab_offset);
            } else if (ch == '\n') {
                cur = pchar - substr_begin;
                break;
            } else if (ch == 0x03/* ETX */) {
                cur = pchar - substr_begin;
                break;
            } else {
                glyph_advance = glyph.horiAdvance / GLYPH_DIV;
                tab_offset = (tab_offset + 1) % GUI_SPACES_PER_TAB;
            }

            float mid = total_advance + glyph_advance * .5f;
            if (mid < mouse_local.x) {
                cur = pchar - substr_begin + 1;
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

        if (full_text_utf.size() > 0) {
            str_begin = 0;//&full_text_utf[0];
            str_end = full_text_utf.size();//substr_begin + full_text_utf.size();
        } else {
            str_begin = 0;
            str_end = 0;
        }
        self_linear_size = full_text_utf.size();
    }

    void backspace() {
        if (head) {
            head->backspace();
            return;
        }

        int at = guiGetTextCursor() - linear_begin;
        if (at < 1) {
            return;
        }
        full_text_utf.erase(full_text_utf.begin() + (at - 1));
        guiAdvanceTextCursor(-1);
    }
    void delete_() {
        if (head) {
            head->delete_();
            return;
        }

        int at = guiGetTextCursor() - linear_begin;
        if (at >= full_text_utf.size()) {
            return;
        }
        if (full_text_utf[at] == 0x03/* ETX */) {
            return;
        }
        full_text_utf.erase(full_text_utf.begin() + at);
    }
    void putChar(uint32_t ch) {
        if (head) {
            head->putChar(ch);
            return;
        }

        int at = guiGetTextCursor() - linear_begin;
        full_text_utf.insert(full_text_utf.begin() + at, ch);
        guiAdvanceTextCursor(1);
    }
    void newline() {
        if (head) {
            head->newline();
            return;
        }

        int at = guiGetTextCursor() - linear_begin;
        full_text_utf.insert(full_text_utf.begin() + at, '\n');
        guiAdvanceTextCursor(1);
    }
    void advanceCursor(int offset) {
        int text_begin = getHead()->linear_begin;
        int text_end = text_begin + getHead()->full_text_utf.size();
        int to_begin = -(guiGetTextCursor() - text_begin);
        int to_end = text_end - guiGetTextCursor() - 1; // - 1 for ETX
        offset = std::max(to_begin, std::min(to_end, offset));
        guiAdvanceTextCursor(offset);
    }
protected:
    enum class TEXT_ALIGNMENT {
        LEFT,
        CENTER,
        RIGHT
    };
    TEXT_ALIGNMENT inner_alignment = TEXT_ALIGNMENT::LEFT;

public:
    GuiTextElement(const std::string& text = "") {
        setSize(gui::perc(100), gui::em(1));
        setTextFromString(text);
    }

    void setContent(const std::string& content) {
        setTextFromString(content);
    }
    std::string getText() const {
        int len = getHead()->full_text_utf.size() - 1; // -1 for ETX
        if (len == -1) {
            return std::string();
        }
        std::string out;
        out.resize(len);
        for (int i = 0; i < len; ++i) {
            out[i] = (char)getHead()->full_text_utf[i];
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
                LOG_DBG(std::format("{:#02x}", ch));
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
                advanceCursor(-1);
                return true;
            }
            case VK_RIGHT: {
                advanceCursor(1);
                return true;
            }
            case VK_UP: {
                // TODO:
                return true;
            }
            case VK_DOWN: {
                // TODO:
                return true;
            }
            }
            }
            break;
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

        const int available_width = client_area.max.x - client_area.min.x;
        int total_advance = 0;

        int tab_offset = 0;

        if (isHead()) {
            str_end = str_begin + full_text_utf.size();
        }

        int ichar = str_begin;
        int word_wrap_end = str_begin;
        int word_wrap_total_advance = total_advance;
        uint32_t lastchar = ' ';
        for (; ichar != str_end; ++ichar) {
            uint32_t ch = getHead()->full_text_utf[ichar];

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

            if (available_width < total_advance + glyph_advance) {
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
            this->hori_advance_px = total_advance;
        } else {
            this->hori_advance_px = word_wrap_total_advance;
            ichar = word_wrap_end;
        }

        int next_begin = ichar;
        int next_end = str_end;
        str_end = ichar;

        self_linear_size = str_end - str_begin;
        linear_end = linear_begin + self_linear_size;

        if (next_begin < next_end) {
            if (!next_line) {
                next_line.reset(new GuiTextElement(head ? head : this, next_begin, next_end, depth + 1));
                next_line->setStyleClasses(getStyleClasses());
                next_line->owner = this;
                next_line->parent = parent;
                next_wrapped = next_line.get();
            }
            next_line->str_begin = next_begin;
            next_line->str_end = next_end;
            next_line->linear_begin = linear_end;
            // Update next_wrapped in case it was set to zero before
            next_wrapped = next_line.get();
        } else if(next_line) {
            // Don't delete next lines, just hide them by zeroing next_wrapped
            // Unused lines will stay in memory, but it's not a big deal
            next_wrapped = 0;// next_line.get();
        }

        //GuiElement::onLayout(rc, flags);
    }
    void onDraw() override {
        GuiElement::onDraw();

        Font* font = getFont();
        gfxm::rect rc = getBoundingRect();
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

        int hori_advance = getInnerAlignmentOffset();
        int char_offset = 0;
        int line_offset = line_height - font->getDescender();
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
            const float x_at = rc.min.x;
            const float y_at = rc.min.y;

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
            /*gfxm::rect rc_line(
                gfxm::vec2(client_area.min.x + px_cursor_at, client_area.min.y),
                gfxm::vec2(client_area.min.x + px_cursor_at, client_area.max.y)
            );*/
            gfxm::rect rc_rect(
                gfxm::vec2(client_area.min.x + px_cursor_at, client_area.min.y),
                gfxm::vec2(client_area.min.x + px_cursor_at, client_area.max.y) + gfxm::vec2(2, 0)
            );
            if ((time(0) - guiGetTextCursorTime()) % 2 == 0) {
                //guiDrawLine(rc_line, 1.f, color);
                guiDrawRect(rc_rect, color);
            }
        }
    }
};
