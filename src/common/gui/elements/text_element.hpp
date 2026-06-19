#pragma once

#include "element.hpp"
#include "gui/gui_text_buffer.hpp"
#include "text_layout/text_layout.hpp"


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

class GuiTextLayout;
class GuiTextElement : public GuiElement {
    friend GuiTextLayout;

    TextLayout text_layout;
    std::string string_utf8;

    bool is_highlighting = false;
    int highlight_begin = 0;
    int highlight_end = 0;

    bool is_read_only = true;
    bool enable_word_wrap = true;
    bool use_elipsis = true; // TODO
    
    float cursor_blink = .0f;

    static TextLayout::HALIGN convertHorizontalAlignmentToTextLayout(GUI_HORIZONTAL_ALIGNMENT halign) {
        switch (halign) {
        case GUI_HORIZONTAL_ALIGNMENT::LEFT: return TextLayout::HALIGN_LEFT;
        case GUI_HORIZONTAL_ALIGNMENT::CENTER: return TextLayout::HALIGN_CENTER;
        case GUI_HORIZONTAL_ALIGNMENT::RIGHT: return TextLayout::HALIGN_RIGHT;
        default:
            assert(false);
            return TextLayout::HALIGN_LEFT;
        }
    }
    static TextLayout::VALIGN convertVerticalAlignmentToTextLayout(GUI_VERTICAL_ALIGNMENT halign) {
        switch (halign) {
        case GUI_VERTICAL_ALIGNMENT::TOP: return TextLayout::VALIGN_TOP;
        case GUI_VERTICAL_ALIGNMENT::CENTER: return TextLayout::VALIGN_CENTER;
        case GUI_VERTICAL_ALIGNMENT::BOTTOM: return TextLayout::VALIGN_BOTTOM;
        default:
            assert(false);
            return TextLayout::VALIGN_TOP;
        }
    }

    void setHighlight(int begin, int end) {
        highlight_begin = begin;
        highlight_end = end;
        text_layout.clearRanges();
        text_layout.addRange(begin, end, 0xFFCCCCCC);
    }
    
    int pickCursorPosition(const gfxm::vec2& mouse_local) {
        return linear_begin + text_layout.hitTest(mouse_local.x, mouse_local.y);
    }
    void setTextFromString(const std::string& text) {
        /*
        string_utf8.resize(text.length() + 1);
        for (int i = 0; i < text.length(); ++i) {
            string_utf8[i] = text[i];
        }
        string_utf8[string_utf8.size() - 1] = 0x03;
        */
        string_utf8 = text;
        self_linear_size = string_utf8.size();

        text_layout.setString(string_utf8.data(), string_utf8.size());
        highlight_begin = gfxm::_min(highlight_begin, int(string_utf8.size()));
        highlight_end = gfxm::_min(highlight_end, int(string_utf8.size()));
    }

    void backspace() {
        int begin = gfxm::_min(highlight_begin, highlight_end);
        int end = gfxm::_max(highlight_begin, highlight_end);
        if (begin != end) {
            string_utf8.erase(string_utf8.begin() + begin, string_utf8.begin() + end);
            setHighlight(begin, begin);
        } else {
            int at = highlight_end;
            if (at < 1) {
                return;
            }
            string_utf8.erase(string_utf8.begin() + (at - 1));
            advanceCursor(-1, false);
        }
        text_layout.setString(string_utf8.data(), string_utf8.size());
        cursor_blink = 1.f;
    }
    void delete_() {
        int begin = gfxm::_min(highlight_begin, highlight_end);
        int end = gfxm::_max(highlight_begin, highlight_end);
        if (begin != end) {
            string_utf8.erase(string_utf8.begin() + begin, string_utf8.begin() + end);
            setHighlight(begin, begin);
        } else {
            int at = highlight_end;
            if (at >= string_utf8.size()) {
                return;
            }/*
            if (string_utf8[at] == 0x03) {
                return;
            }*/
            string_utf8.erase(string_utf8.begin() + at);
        }
        text_layout.setString(string_utf8.data(), string_utf8.size());
        cursor_blink = 1.f;
    }
    void putChar(uint32_t ch) {
        int begin = gfxm::_min(highlight_begin, highlight_end);
        int end = gfxm::_max(highlight_begin, highlight_end);
        if (begin != end) {
            string_utf8.erase(string_utf8.begin() + begin, string_utf8.begin() + end);
            highlight_begin = begin;
            highlight_end = begin;
        }

        int at = highlight_end;
        string_utf8.insert(string_utf8.begin() + at, ch);
        advanceCursor(1, false);
        text_layout.setString(string_utf8.data(), string_utf8.size());
        cursor_blink = 1.f;
    }
    void newline() {
        int begin = gfxm::_min(highlight_begin, highlight_end);
        int end = gfxm::_max(highlight_begin, highlight_end);
        if (begin != end) {
            string_utf8.erase(string_utf8.begin() + begin, string_utf8.begin() + end);
            highlight_begin = begin;
            highlight_end = begin;
        }

        int at = begin;
        string_utf8.insert(string_utf8.begin() + at, '\n');
        advanceCursor(1, false);
        text_layout.setString(string_utf8.data(), string_utf8.size());
        cursor_blink = 1.f;
    }
    void setCursor(int at) {
        highlight_begin = at;
        highlight_end = at;
        setHighlight(highlight_begin, highlight_end);
        cursor_blink = 1.f;
    }
    void advanceCursor(int offset, bool highlight = false) {
        int new_cur = highlight_end + offset;
        new_cur = std::min(int(string_utf8.size()), std::max(0, new_cur));
        highlight_end = new_cur;
        if (!highlight) {
            highlight_begin = highlight_end;
        }
        setHighlight(highlight_begin, highlight_end);
        cursor_blink = 1.f;
    }
    void jumpLine(int offset, bool highlight = false) {
        /*
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
        */
        cursor_blink = 1.f;
    }
public:
    GuiTextElement(const std::string& text = "");

    // TODO: Should probably be a GUI_FLAG_READ_ONLY
    void setReadOnly(bool val) {
        is_read_only = val;
    }
    void setContent(const std::string& content) {
        setTextFromString(content);
    }

    void cursorToEnd() {
        // TODO: Handle mismatch between raw bytes and decoded utf8 positions
        setCursor(string_utf8.size());
        cursor_blink = 1.f;
    }

    std::string getText() const {
        return string_utf8;
    }

    void onFontChanged(Font* font) override;

    int measureWidth(const std::optional<int>& height) override;
    int measureHeight(const std::optional<int>& width) override;
    void layout_2(const gui_layout_context& ctx) override;
    void onUpdate(float dt) override;

    void onDraw() override {
        GuiElement::onDraw();

        Font* font = getFont();

        for (int i = 0; i < text_layout.spans.size(); ++i) {
            const auto& sp = text_layout.spans[i];
            guiDrawRectRound(sp.rc, 5, 5, 5, 5, sp.color);
        }

        std::vector<GuiTextVertex> vertices;
        for (int i = 0; i < text_layout.glyphs.size(); ++i) {
            const auto& g = text_layout.glyphs[i];
            if (!g.renderable) {
                continue;
            }
            const auto q = g.makeQuad();

            uint32_t color = g.color;

            // shadow
            const gfxm::vec3 shadow_offs = gfxm::vec3(1.f, 1.f, .0f);
            vertices.insert(
                vertices.end(),
                {
                    GuiTextVertex{ q.pos[0] + shadow_offs, q.uv[0], 0xFF000000, q.lut_values[0] },
                    GuiTextVertex{ q.pos[1] + shadow_offs, q.uv[1], 0xFF000000, q.lut_values[1] },
                    GuiTextVertex{ q.pos[2] + shadow_offs, q.uv[2], 0xFF000000, q.lut_values[2] },
                    GuiTextVertex{ q.pos[1] + shadow_offs, q.uv[1], 0xFF000000, q.lut_values[1] },
                    GuiTextVertex{ q.pos[3] + shadow_offs, q.uv[3], 0xFF000000, q.lut_values[3] },
                    GuiTextVertex{ q.pos[2] + shadow_offs, q.uv[2], 0xFF000000, q.lut_values[2] }
                }
            );
            // text
            vertices.insert(
                vertices.end(),
                {
                    GuiTextVertex{ q.pos[0], q.uv[0], color, q.lut_values[0] },
                    GuiTextVertex{ q.pos[1], q.uv[1], color, q.lut_values[1] },
                    GuiTextVertex{ q.pos[2], q.uv[2], color, q.lut_values[2] },
                    GuiTextVertex{ q.pos[1], q.uv[1], color, q.lut_values[1] },
                    GuiTextVertex{ q.pos[3], q.uv[3], color, q.lut_values[3] },
                    GuiTextVertex{ q.pos[2], q.uv[2], color, q.lut_values[2] }
                }
            );
        }

        guiDrawText2(vertices.data(), vertices.size(), font);

        if (isFocused() && highlight_begin == highlight_end && highlight_end <= text_layout.glyphs.size()) {
            int x_at = 0;
            const TextLayout::Line* ln = nullptr;
            if(highlight_end < text_layout.glyphs.size()) {
                const auto& g = text_layout.glyphs[highlight_end];
                ln = &text_layout.lines_wrapped[g.line_idx];
                x_at = g.x_left_side;
            } else {
                ln = &text_layout.lines_wrapped[0];
                x_at = ln->x_end;
            }

            gfxm::vec2 a(x_at, ln->y_baseline - text_layout.ascender);
            gfxm::vec2 b(x_at, ln->y_baseline + text_layout.descender);

            guiDrawRect(
                gfxm::rect(
                    gfxm::vec2(x_at, ln->y_baseline - text_layout.ascender),
                    gfxm::vec2(x_at + 3, ln->y_baseline + text_layout.descender)
                ),
                gfxm::hsv2rgb32(.0f, .0f, 1.f, cursor_blink)
            );
            //guiDrawLine(a, b, 2, 0xFFFFFFFF);
        }
    }
};
