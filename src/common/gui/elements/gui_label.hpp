#pragma once

#include "gui/elements/gui_element.hpp"


class GuiLabel : public GuiElement {
    GuiTextBuffer text_caption;
    gfxm::vec2 pos_caption;
public:
    GuiLabel(const char* caption = "Label")
    : text_caption(guiGetDefaultFont()) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        text_caption.replaceAll(caption, strlen(caption));
        text_caption.prepareDraw(guiGetDefaultFont(), false);
        size.x = text_caption.getBoundingSize().x + GUI_PADDING * 2.f;
        size.y = guiGetDefaultFont()->font->getLineHeight() * 2.f;
    }

    void setCaption(const char* cap) {
        text_caption.replaceAll(cap, strlen(cap));
    }

    GuiHitResult onHitTest(int x, int y) override {
        return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = gfxm::rect(
            rc.min,
            rc.min + size
        );
        client_area = rc_bounds;
        pos_caption = rc_bounds.min;
        pos_caption.x += GUI_PADDING;
        const float content_y_offset =  size.y * .5f - (text_caption.font->font->getAscender() + text_caption.font->font->getDescender()) * .5f;
        pos_caption.y += content_y_offset;

        text_caption.prepareDraw(guiGetCurrentFont(), false);
    }
    void onDraw() override {
        text_caption.draw(pos_caption, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};