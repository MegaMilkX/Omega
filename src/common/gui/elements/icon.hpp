#pragma once

#include "element.hpp"
#include "gui/gui_icon.hpp"


class GuiIconElement : public GuiElement {
    GuiIcon* icon = 0;
public:
    void setIcon(GuiIcon* icn) {
        icon = icn;
    }
    void onDraw() override {
        GuiElement::onDraw();
        gfxm::rect rc = getBoundingRect();
        uint32_t color = GUI_COL_WHITE;
        auto color_style = getStyleComponent<gui::style_color>();
        if (color_style && color_style->color.has_value()) {
            color = color_style->color.value();
        }

        if (icon) {
            icon->draw(rc, color);
        }
    }
};