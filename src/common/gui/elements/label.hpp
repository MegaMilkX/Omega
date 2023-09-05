#pragma once

#include "gui/elements/element.hpp"


class GuiLabel : public GuiElement {
    GuiTextBuffer text_caption;
    gfxm::vec2 pos_caption;
public:
    GuiLabel(const char* caption = "Label") {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        setStyleClasses({ "control" });
        Font* font = getFont();
        text_caption.replaceAll(font, caption, strlen(caption));
        text_caption.prepareDraw(font, false);
        size.x = text_caption.getBoundingSize().x + GUI_PADDING * 2.f;
        size.y = font->getLineHeight() * 2.f;
    }

    void setCaption(const char* cap) {
        text_caption.replaceAll(getFont(), cap, strlen(cap));
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        return;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = getFont();
        gfxm::vec2 px_size = gui_to_px(size, font, gfxm::vec2(rc.max.x - rc.min.x, rc.max.y - rc.min.y));
        rc_bounds = gfxm::rect(
            rc.min,
            rc.min + px_size
        );
        client_area = rc_bounds;
        pos_caption = rc_bounds.min;
        pos_caption.x += GUI_PADDING;
        const float content_y_offset = px_size.y * .5f - (font->getAscender() + font->getDescender()) * .5f;
        pos_caption.y += content_y_offset;

        text_caption.prepareDraw(font, false);
    }
    void onDraw() override {
        text_caption.draw(getFont(), pos_caption, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};