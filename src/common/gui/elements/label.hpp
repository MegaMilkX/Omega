#pragma once

#include "gui/elements/element.hpp"


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

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        return;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        gfxm::vec2 px_size = gui_to_px(size, guiGetCurrentFont(), gfxm::vec2(rc.max.x - rc.min.x, rc.max.y - rc.min.y));
        rc_bounds = gfxm::rect(
            rc.min,
            rc.min + px_size
        );
        client_area = rc_bounds;
        pos_caption = rc_bounds.min;
        pos_caption.x += GUI_PADDING;
        const float content_y_offset = px_size.y * .5f - (text_caption.font->font->getAscender() + text_caption.font->font->getDescender()) * .5f;
        pos_caption.y += content_y_offset;

        text_caption.prepareDraw(guiGetCurrentFont(), false);
    }
    void onDraw() override {
        text_caption.draw(pos_caption, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};