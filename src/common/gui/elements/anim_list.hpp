#pragma once


#include "gui/elements/element.hpp"


class GuiAnimSyncListToolbar : public GuiElement {
public:
    GuiAnimSyncListToolbar();
    
    void onDraw() override {
        guiDrawRect(rc_bounds, GUI_COL_BUTTON);
        GuiElement::onDraw();
    }
};
class GuiAnimSyncListGroup : public GuiElement {
public:
    GuiAnimSyncListGroup() {
        setSize(0, gui::em(1.5));
        setMinSize(0, gui::em(1.5));
        content_padding = gfxm::rect(GUI_PADDING, 20.f, GUI_PADDING, GUI_PADDING);
        overflow = GUI_OVERFLOW_FIT;
    }
    void onDraw() override {
        gfxm::rect rc = rc_bounds;
        rc.max.y = rc.min.y + 20.f;
        guiDrawRectRound(rc, 10.f, GUI_COL_BG);
        rc.min.x += GUI_MARGIN;
        guiDrawText(rc, "SyncGroup", guiGetCurrentFont(), GUI_VCENTER | GUI_LEFT, GUI_COL_TEXT);

        GuiElement::onDraw();
    }
};
class GuiAnimSyncListItem : public GuiElement {
public:
    GuiAnimSyncListItem() {
        setSize(0, gui::em(1));
    }
    void onDraw() override {
        if (isHovered()) {
            guiDrawRect(rc_bounds, GUI_COL_BUTTON_HOVER);
        }
        gfxm::rect rc = rc_bounds;
        rc.min.x += GUI_MARGIN;
        guiDrawText(rc, "AnimItem", guiGetCurrentFont(), GUI_VCENTER | GUI_LEFT, GUI_COL_TEXT);
    }
};
class GuiAnimationSyncList : public GuiElement {
public:
    GuiAnimationSyncList();

    void onDraw() override {
        guiDrawRect(rc_bounds, GUI_COL_BG_INNER);
        GuiElement::onDraw();

        guiDrawRectLine(rc_bounds, GUI_COL_BORDER);
    }
};