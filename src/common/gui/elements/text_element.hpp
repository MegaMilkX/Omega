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

class GuiTextElement : public GuiElement {
    TextLayout text_layout;
    std::string string_utf8;

    bool is_highlighting = false;
    int highlight_begin = 0;
    int highlight_end = 0;

    bool is_read_only = true;
    bool enable_word_wrap = true;
    bool use_elipsis = true; // TODO
    
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
        string_utf8.resize(text.length() + 1);
        for (int i = 0; i < text.length(); ++i) {
            string_utf8[i] = text[i];
        }
        string_utf8[string_utf8.size() - 1] = 0x03;

        self_linear_size = string_utf8.size();
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
            }
            if (string_utf8[at] == 0x03/* ETX */) {
                return;
            }
            string_utf8.erase(string_utf8.begin() + at);
        }
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
    }
    void setCursor(int at) {
        highlight_begin = at;
        highlight_end = at;
        setHighlight(highlight_begin, highlight_end);
    }
    void advanceCursor(int offset, bool highlight = false) {
        int new_cur = highlight_end + offset;
        new_cur = std::min(int(string_utf8.size()), std::max(0, new_cur));
        highlight_end = new_cur;
        if (!highlight) {
            highlight_begin = highlight_end;
        }
        setHighlight(highlight_begin, highlight_end);
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
    }
public:
    GuiTextElement(const std::string& text = "") {
        setSize(gui::content(), gui::content());
        setTextFromString(text);

        subscribe<GuiEvt_Focus>([this](const GuiEvt_Focus& e) {
            if(!is_read_only) {
                e.new_focused = this;
            } else {
                e.consume = false;
            }
        });
        subscribe<GuiEvt_Unfocus>([this](const GuiEvt_Unfocus& e) {
            setHighlight(0, 0);
        });

        subscribe<GuiEvt_MouseBtn>([this](const GuiEvt_MouseBtn& e) {
            if (is_read_only) {
                e.consume = false;
                return;
            }
            if (e.btn == GUI_MOUSE_LEFT) {
                if(e.state == GUI_KEY_DOWN) {
                    is_highlighting = true;
                    auto mouse = guiGetMousePos() - getGlobalPosition();
                    highlight_begin = text_layout.hitTest(mouse.x, mouse.y);
                    highlight_end = highlight_begin;
                    text_layout.clearRanges();
                } else if (e.state == GUI_KEY_UP) {
                    is_highlighting = false;
                }
            } else {
                e.consume = false;
            }
        });
        
        subscribe<GuiEvt_MouseMove>([this](const GuiEvt_MouseMove& e) {
            if (is_highlighting) {
                gfxm::vec2 lclmouse(e.x, e.y);
                lclmouse = lclmouse - getGlobalPosition();
                highlight_end = text_layout.hitTest(lclmouse.x, lclmouse.y);
                text_layout.clearRanges();
                text_layout.addRange(highlight_begin, highlight_end, 0xFFCCCCCC);
            }
        });

        subscribe<GuiEvt_Unichar>([this](const GuiEvt_Unichar& e) {
            switch (e.ch) {
            case uint32_t(GUI_CHAR::BACKSPACE): {
                backspace();
                return true;
            }
            case uint32_t(GUI_CHAR::RETURN): {
                newline();
                return true;
            }
            default: {
                uint32_t ch = e.ch;
                LOG_DBG(std::format("GuiEvt_Unichar: {:#02x}", ch));
                if (ch > 0x1F || ch == 0x0A) {
                    putChar(ch);
                }
                return true;
            }
            }
        });
    }

    // TODO: Should probably be a GUI_FLAG_READ_ONLY
    void setReadOnly(bool val) {
        is_read_only = val;
    }
    void setContent(const std::string& content) {
        setTextFromString(content);
    }

    std::string getText() const {
        return string_utf8;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
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
        }
        return GuiElement::onMessage(msg, params);
    }

    void onLayout(const gui_layout_context& ctx) {
        Font* font = getFont();

        if (ctx.flags & GUI_LAYOUT_WIDTH_PASS) {
            rc_bounds.min.x = 0;
            rc_bounds.max.x = roundf(ctx.width.value_or(0));
            client_area.min.x = rc_bounds.min.x;
            client_area.max.x = rc_bounds.max.x;

            auto style = getStyle();

            uint32_t color = 0xFFFFFFFF;
            auto style_color = style->get_component<gui::style_color>();
            if (style_color) {
                color = style_color->color.value(0xFFFFFFFF);
            }
            text_layout.base_color = color;

            GUI_HORIZONTAL_ALIGNMENT halign = GUI_HORIZONTAL_ALIGNMENT::LEFT;
            gui_rect padding;
            auto style_box = style->get_component<gui::style_box>();
            if (style_box) {
                halign = style_box->horizontal_align.value(GUI_HORIZONTAL_ALIGNMENT::LEFT);
                padding = style_box->padding.value(gui_rect(gui::px(0), gui::px(0), gui::px(0), gui::px(0)));                    
            }
            TextLayout::HALIGN tl_halign = convertHorizontalAlignmentToTextLayout(halign);

            gui_rect border_thickness;
            auto style_border = style->get_component<gui::style_border>();
            if (style_border) {
                border_thickness = gui_rect(
                    style_border->thickness_left.value(0),
                    style_border->thickness_top.value(0),
                    style_border->thickness_right.value(0),
                    style_border->thickness_bottom.value(0)
                );
            }

            if(ctx.flags & GUI_LAYOUT_FIT_CONTENT) {
                text_layout.build(string_utf8, font, -1);
                text_layout.alignHorizontal(tl_halign, -1);

                gfxm::rect px_padding = gui_to_px(padding, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));
                gfxm::rect px_border = gui_to_px(border_thickness, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));

                text_layout.padHorizontal(px_padding.min.x + px_border.min.x, px_padding.max.x + px_border.max.x);
                client_area.max.x = client_area.min.x + text_layout.bounding_width + px_padding.min.x + px_padding.max.x;
                rc_bounds.max.x = rc_bounds.min.x + text_layout.bounding_width;
            } else {
                gfxm::rect px_padding = gui_to_px(padding, font->getLineHeight(), gfxm::vec2(ctx.width.value_or(0), 0));
                gfxm::rect px_border = gui_to_px(border_thickness, font->getLineHeight(), gfxm::vec2(ctx.width.value_or(0), 0));
                client_area.min.x += px_padding.min.x;
                client_area.max.x -= px_padding.max.x;

                int box_width = ctx.width.value_or(0);
                box_width = gfxm::_max(.0f, box_width - (px_padding.min.x + px_padding.max.x));

                text_layout.build(string_utf8, font, box_width);
                text_layout.alignHorizontal(tl_halign, box_width);

                text_layout.padHorizontal(px_padding.min.x + px_border.min.x, px_padding.max.x + px_border.max.x);
            }

            self_linear_size = string_utf8.size();
            linear_end = linear_begin + self_linear_size;
        }

        if (ctx.flags & GUI_LAYOUT_HEIGHT_PASS) {
            rc_bounds.min.y = 0;
            rc_bounds.max.y = roundf(ctx.height.value_or(0));
            client_area.min.y = rc_bounds.min.y;
            client_area.max.y = rc_bounds.max.y;

            GUI_VERTICAL_ALIGNMENT valign = GUI_VERTICAL_ALIGNMENT::TOP;
            gui_rect padding;
            auto style = getStyle();
            auto style_box = style->get_component<gui::style_box>();
            if (style_box) {
                valign = style_box->vertical_align.value(GUI_VERTICAL_ALIGNMENT::TOP);
                padding = style_box->padding.value(gui_rect(gui::px(0), gui::px(0), gui::px(0), gui::px(0)));
            }
            TextLayout::VALIGN tl_valign = convertVerticalAlignmentToTextLayout(valign);

            gui_rect border_thickness;
            auto style_border = style->get_component<gui::style_border>();
            if (style_border) {
                border_thickness = gui_rect(
                    style_border->thickness_left.value(0),
                    style_border->thickness_top.value(0),
                    style_border->thickness_right.value(0),
                    style_border->thickness_bottom.value(0)
                );
            }

            gfxm::rect px_padding = gui_to_px(padding, font->getLineHeight(), gfxm::vec2(0, text_layout.bounding_height));
            gfxm::rect px_border = gui_to_px(border_thickness, font->getLineHeight(), gfxm::vec2(0, text_layout.bounding_height));
            //client_area.min.y += px_padding.min.y;
            //client_area.max.y -= px_padding.max.y;

            if ((ctx.flags & GUI_LAYOUT_FIT_CONTENT) == 0) {
                text_layout.alignVertical(tl_valign, ctx.height.value_or(0) - px_padding.min.y - px_padding.max.y);
            }

            text_layout.padVertical(px_padding.min.y + px_border.min.y, px_padding.max.y + px_border.max.y);

            if (ctx.flags & GUI_LAYOUT_FIT_CONTENT) {
                client_area.max.y = client_area.min.y + text_layout.bounding_height + px_padding.min.y + px_padding.max.y + px_border.min.y + px_border.max.y;
                rc_bounds.max.y = rc_bounds.min.y + text_layout.bounding_height + px_padding.min.y + px_padding.max.y + px_border.min.y + px_border.max.y;
            }
        }

        if (ctx.flags & GUI_LAYOUT_POSITION_PASS) {

        }
    }

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

        if (isFocused() && highlight_begin == highlight_end && highlight_end < string_utf8.size()) {
            const auto& g = text_layout.glyphs[highlight_end];
            const auto& ln = text_layout.lines[g.line_idx];

            gfxm::vec2 a(g.x_left_side, ln.y_baseline - text_layout.ascender);
            gfxm::vec2 b(g.x_left_side, ln.y_baseline + text_layout.descender);

            guiDrawLine(a, b, 2, 0xFFFFFFFF);
        }
    }
};
