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
    GuiElement* head = 0;
    GuiElement* content_box = 0;
public:
    GuiAnimSyncListGroup() {
        setSize(gui::fill(), gui::content());

        head = new GuiElement;
        head->setSize(gui::perc(100), gui::em(1.5));
        content_box = new GuiElement;
        content_box->setSize(gui::fill(), gui::content());
        this->content = content_box;
        _addChild(head);
        _addChild(content_box);
    }
    void onDraw() override {
        gfxm::rect rc = head->getBoundingRect();
        guiDrawRectRound(rc, 10.f, GUI_COL_BG);
        rc.min.x += GUI_MARGIN;
        guiDrawText(rc, "SyncGroup", getFont(), GUI_VCENTER | GUI_LEFT, GUI_COL_TEXT);

        GuiElement::onDraw();
    }
};
class GuiAnimSyncListItem : public GuiElement {
public:
    GuiAnimSyncListItem() {
        setSize(0, gui::em(1.5));
    }
    void onDraw() override {
        if (isHovered()) {
            guiDrawRect(rc_bounds, GUI_COL_BUTTON);
        }
        gfxm::rect rc = rc_bounds;
        rc.min.x += GUI_MARGIN;
        guiDrawText(rc, "AnimItem", getFont(), GUI_VCENTER | GUI_LEFT, GUI_COL_TEXT);
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