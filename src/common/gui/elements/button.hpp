#pragma once

#include "gui/elements/element.hpp"
#include "gui/gui_icon.hpp"
#include "gui/gui_text_buffer.hpp"


class GuiButton : public GuiElement {
    GuiTextBuffer caption;
    int caption_len = 0;

    gfxm::vec2 text_pos;
    gfxm::vec2 icon_pos;
    const GuiIcon* icon = 0;
    std::function<void(void)> on_click;

    void updateSize() {
        setSize(gui::px(caption.getBoundingSize().x + GUI_MARGIN * 2), gui::em(2));
    }
public:
    GuiButton(
        const char* caption = "Button",
        const GuiIcon* icon = 0,
        std::function<void(void)> on_click = nullptr
    )
    : caption(guiGetDefaultFont()), on_click(on_click) {
        setSize(0.0f, 30.0f);
        caption_len = strlen(caption);
        this->caption.replaceAll(caption, caption_len);
        this->icon = icon;
        updateSize();
    }

    void setCaption(const char* cap) {
        caption.replaceAll(cap, strlen(cap));
        updateSize();
    }
    void setIcon(const GuiIcon* icon) {
        this->icon = icon;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::DBL_LCLICK:
        case GUI_MSG::LCLICK: {
            notifyOwner(GUI_NOTIFY::BUTTON_CLICKED, this);
            if (on_click) {
                on_click();
            }
            } return true;
        }

        return GuiElement::onMessage(msg, params);
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;

        float icon_offs = .0f;
        if (icon) {
            icon_offs = guiGetCurrentFont()->font->getLineHeight();
        }
        float text_width = .0f;
        if (caption_len) {
            text_width = caption.getBoundingSize().x;
        }
        rc_bounds = gfxm::rect(
            rc.min,
            rc.min + gfxm::vec2(icon_offs + GUI_PADDING * 4.f + text_width, font->getLineHeight() * 2.f)
        );
        client_area = rc_bounds;

        caption.prepareDraw(guiGetCurrentFont(), false);
        text_pos = guiCalcTextPosInRect(gfxm::rect(gfxm::vec2(0, 0), caption.getBoundingSize()), client_area, 0, gfxm::rect(0, 0, 0, 0), font);
        icon_pos = text_pos;
        text_pos.x += icon_offs;

        //box.setSize(gui_vec2(rc_bounds.max - rc_bounds.min, gui_pixel));
    }

    void onDraw() override {
        Font* font = guiGetCurrentFont()->font;

        uint32_t col = GUI_COL_BUTTON;
        uint32_t col_highlight = GUI_COL_BUTTON_HIGHLIGHT;
        uint32_t col_shadow = GUI_COL_BUTTON_SHADOW;
        gfxm::vec2 text_offs;
        if (isPressed()) {
            col = GUI_COL_BUTTON_SHADOW;
            col_highlight = GUI_COL_BUTTON_SHADOW;
            col_shadow = GUI_COL_BUTTON_HIGHLIGHT;
            text_offs.y = 2.f;
        } else if (isHovered()) {
            col = GUI_COL_BUTTON_HOVER;
        }
        //guiDrawRectRound(client_area, GUI_PADDING * 2.f, col_shadow);
        //guiDrawRectRound(gfxm::rect(client_area.min, client_area.max - gfxm::vec2(0, 5)), GUI_PADDING * 2.f, col_highlight);
        /*guiDrawRectRound(
            gfxm::rect(client_area.min + gfxm::vec2(0, 2), client_area.max - gfxm::vec2(0, 2)),
            GUI_PADDING * 2.f, col
        );*/
        guiDrawRectRound(
            gfxm::rect(client_area.min, client_area.max),
            GUI_PADDING * 2.f, col
        );

        auto fnt = guiGetCurrentFont();
        //guiDrawRect(gfxm::rect(icon_pos, icon_pos + gfxm::vec2(fnt->font->getLineHeight(), fnt->font->getLineHeight())), GUI_COL_WHITE);
        if (icon) {
            icon->draw(gfxm::rect(icon_pos, icon_pos + gfxm::vec2(fnt->font->getLineHeight(), fnt->font->getLineHeight())), GUI_COL_WHITE);
        }
        caption.draw(text_pos + text_offs, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};

class GuiIconButton : public GuiElement {
    const GuiIcon* icon = 0;
public:
    GuiIconButton(const GuiIcon* icon) {
        setIcon(icon);
        setSize(gui::em(2), gui::em(2));
    }

    void setIcon(const GuiIcon* icon) {
        this->icon = icon;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::DBL_LCLICK:
        case GUI_MSG::LCLICK: {
            notifyOwner(GUI_NOTIFY::BUTTON_CLICKED, this);
        } return true;
        }

        return GuiElement::onMessage(msg, params);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;
        rc_bounds = gfxm::rect(
            rc.min,
            rc.min + gfxm::vec2(font->getLineHeight() * 2.f, font->getLineHeight() * 2.f)
        );
        client_area = rc_bounds;
    }

    void onDraw() override {
        Font* font = guiGetCurrentFont()->font;

        uint32_t col = GUI_COL_BUTTON;
        if (isPressed()) {
            col = GUI_COL_ACCENT;
        } else if (isHovered()) {
            col = GUI_COL_BUTTON_HOVER;
        }
        guiDrawRectRound(client_area, GUI_PADDING * 2.f, col);

        if (icon) {
            icon->draw(client_area, GUI_COL_WHITE);
        }
    }
};
