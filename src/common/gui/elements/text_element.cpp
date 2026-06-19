#include "text_element.hpp"

#include "gui/gui_system.hpp"
#include "gui/layout/text_layout.hpp"

GuiTextElement::GuiTextElement(const std::string& text) {
    setSize(gui::content(), gui::content());
    setTextFromString(text);

    subscribe<GuiEvt_Focus>([this](const GuiEvt_Focus& e) {
        if(!is_read_only) {
            e.new_focused = this;
            guiCancelTick(this);
            guiScheduleTick(this, 1.f / 15.f);
            cursor_blink = 1.f;
        } else {
            e.consume = false;
        }
    });
    subscribe<GuiEvt_Unfocus>([this](const GuiEvt_Unfocus& e) {
        setHighlight(0, 0);
        cursor_blink = .0f;
        guiCancelTick(this);
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

    auto keydown_hdl = getHandler<GuiEvt_KeyDown>();
    subscribe<GuiEvt_KeyDown>([this, keydown_hdl](const GuiEvt_KeyDown& e) {
        switch (e.vkey) {
        case VK_DELETE: delete_(); return;
        case VK_LEFT: advanceCursor(-1, guiIsModifierKeyPressed(GUI_KEY_SHIFT)); return;
        case VK_RIGHT: advanceCursor(1, guiIsModifierKeyPressed(GUI_KEY_SHIFT)); return;
        case VK_UP: jumpLine(-1, guiIsModifierKeyPressed(GUI_KEY_SHIFT)); return;
        case VK_DOWN: jumpLine(1, guiIsModifierKeyPressed(GUI_KEY_SHIFT)); return;
        case VK_ESCAPE:
            guiUnfocusWindow(this);
            return;
        }
        keydown_hdl.invoke(e);
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
            //LOG_DBG(std::format("GuiEvt_Unichar: {:#02x}", ch));
            if (ch > 0x1F || ch == 0x0A) {
                putChar(ch);
            }
            return true;
        }
        }
    });
}

void GuiTextElement::onFontChanged(Font* font) {
    text_layout.setFont(font);
}

int GuiTextElement::measureWidth(const std::optional<int>& height_constraint) {
    Font* font = getFont();
    auto style = getStyle();

    gui_rect padding;
    auto style_box = style->get_component<gui::style_box>();
    if (style_box) {
        padding = style_box->padding.value(gui_rect(gui::px(0), gui::px(0), gui::px(0), gui::px(0)));                    
    }
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
    gfxm::rect px_padding = gui_to_px(padding, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));
    gfxm::rect px_border = gui_to_px(border_thickness, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));

    text_layout.setPadding(
        px_padding.min.x + px_border.min.x, px_padding.max.x + px_border.max.x,
        px_padding.min.y + px_border.min.y, px_padding.max.y + px_border.max.y
    );
    text_layout.setWidth(std::nullopt);
    text_layout.build(TextLayout::BuildMode::TellWidth);

    return text_layout.bounding_width;
}
int GuiTextElement::measureHeight(const std::optional<int>& width_constraint) {
    Font* font = getFont();
    auto style = getStyle();

    gui_rect padding;
    auto style_box = style->get_component<gui::style_box>();
    if (style_box) {
        padding = style_box->padding.value(gui_rect(gui::px(0), gui::px(0), gui::px(0), gui::px(0)));                    
    }
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
    gfxm::rect px_padding = gui_to_px(padding, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));
    gfxm::rect px_border = gui_to_px(border_thickness, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));

    text_layout.setPadding(
        px_padding.min.x + px_border.min.x, px_padding.max.x + px_border.max.x,
        px_padding.min.y + px_border.min.y, px_padding.max.y + px_border.max.y
    );
    text_layout.setWidth(width_constraint);
    text_layout.build(TextLayout::BuildMode::TellHeight);

    return text_layout.bounding_height;
}
void GuiTextElement::layout_2(const gui_layout_context& ctx) {
    Font* font = getFont();
    auto style = getStyle();
    
    uint32_t color = 0xFFFFFFFF;
    auto style_color = style->get_component<gui::style_color>();
    if (style_color) {
        color = style_color->color.value(0xFFFFFFFF);
    }
    text_layout.base_color = color;

    GUI_HORIZONTAL_ALIGNMENT halign = GUI_HORIZONTAL_ALIGNMENT::LEFT;
    GUI_VERTICAL_ALIGNMENT valign = GUI_VERTICAL_ALIGNMENT::TOP;
    gui_rect padding;
    auto style_box = style->get_component<gui::style_box>();
    if (style_box) {
        halign = style_box->horizontal_align.value(GUI_HORIZONTAL_ALIGNMENT::LEFT);
        valign = style_box->vertical_align.value(GUI_VERTICAL_ALIGNMENT::TOP);
        padding = style_box->padding.value(gui_rect(gui::px(0), gui::px(0), gui::px(0), gui::px(0)));                    
    }
    TextLayout::HALIGN tl_halign = convertHorizontalAlignmentToTextLayout(halign);
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

    gfxm::rect px_padding = gui_to_px(padding, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));
    gfxm::rect px_border = gui_to_px(border_thickness, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));

    text_layout.setPadding(
        px_padding.min.x + px_border.min.x, px_padding.max.x + px_border.max.x,
        px_padding.min.y + px_border.min.y, px_padding.max.y + px_border.max.y
    );
    text_layout.setWidth(ctx.width);
    text_layout.setHeight(ctx.height);
    text_layout.setHAlign(tl_halign);
    text_layout.setVAlign(tl_valign);
    text_layout.build();

    rc_bounds = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(text_layout.bounding_width, text_layout.bounding_height));
    client_area = rc_bounds;
}

void GuiTextElement::onUpdate(float dt) {
    if (!isFocused()) {
        cursor_blink = .0f;
        return;
    }
    guiScheduleTick(this, 1.f / 15.f);
    cursor_blink -= dt;
    if (cursor_blink <= .0f) {
        cursor_blink = 1.f;
    }
}

