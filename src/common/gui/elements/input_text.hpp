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
        setSize(gui::perc(100), gui::em(2));
        setStyleClasses({ "control" });

        label.setOwner(this);
        label.setParent(this);
        box.setOwner(this);
        box.setParent(this);
    }

    std::string getText() { return box.getText(); }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }

        box.hitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        Font* font = getFont();

        const float text_box_height = font->getLineHeight() * 2.0f;
        setHeight(text_box_height);
        rc_bounds = gfxm::rect(
            gfxm::vec2(0, 0),
            gfxm::vec2(extents.x, text_box_height)
        );
        client_area = rc_bounds;

        gfxm::rect rc_left, rc_right;
        rc_left = client_area;
        guiLayoutSplitRect2XRatio(rc_left, rc_right, .25f);
        label.layout_position = rc_left.min;
        label.layout(gfxm::rect_size(rc_left), flags);
        box.layout_position = rc_right.min;
        box.layout(gfxm::rect_size(rc_right), flags);
    }
    void onDraw() override {
        label.draw();
        box.draw();
    }
};
