#pragma once

#include "gui/elements/label.hpp"
#include "gui/elements/input_text_line.hpp"
#include "gui/elements/input.hpp"


class GuiInputText : public GuiElement {
    GuiLabel label;
    GuiInputTextLine box;
public:
    GuiInputText(const char* caption = "InputText", const char* text = "Text")
        : label(caption), box(text) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        label.setOwner(this);
        label.setParent(this);
        box.setOwner(this);
        box.setParent(this);
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }

        box.onHitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = guiGetCurrentFont()->font;

        const float text_box_height = font->getLineHeight() * 2.0f;
        setHeight(text_box_height);
        rc_bounds = gfxm::rect(
            rc.min,
            gfxm::vec2(rc.max.x, rc.min.y + text_box_height)
        );
        client_area = rc_bounds;

        gfxm::rect rc_left, rc_right;
        rc_left = client_area;
        guiLayoutSplitRect2XRatio(rc_left, rc_right, .25f);
        label.layout(rc_left, flags);
        box.layout(rc_right, flags);
    }
    void onDraw() override {
        label.draw();
        box.draw();
    }
};