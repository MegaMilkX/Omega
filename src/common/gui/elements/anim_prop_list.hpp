#pragma once

#include "gui/elements/element.hpp"



class GuiAnimPropListItem : public GuiElement {
public:
    GuiAnimPropListItem() {
        setSize(0, gui::em(1.5));
        margin = gfxm::rect(0, 0, 0, 0);
    }
    void onDraw() override {
        if (isHovered()) {
            guiDrawRect(rc_bounds, GUI_COL_BUTTON);
        }
        gfxm::rect rc = rc_bounds;
        rc.min.x += GUI_MARGIN;
        guiDrawText(rc, "float\tvelocity\t0.01", guiGetCurrentFont(), GUI_VCENTER | GUI_LEFT, GUI_COL_TEXT);
    }
};


class GuiAnimPropListToolbar : public GuiElement {
public:
    GuiAnimPropListToolbar();

    void onDraw() override {
        guiDrawRect(rc_bounds, GUI_COL_BUTTON);
        GuiElement::onDraw();
    }
};


class GuiAnimationPropList : public GuiElement {
public:
    GuiAnimationPropList();

    void onDraw() override {
        guiDrawRect(rc_bounds, GUI_COL_BG_INNER);
        GuiElement::onDraw();

        guiDrawRectLine(rc_bounds, GUI_COL_BORDER);
    }
};