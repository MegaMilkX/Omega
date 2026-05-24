#pragma once

#include "gui/elements/element.hpp"
#include "IconsForkAwesome.h"


class GuiCollapsingHeader : public GuiElement {
    bool is_open = false;
    std::string caption;
    GuiElement header;
    GuiElement content_box;
public:
    void* user_ptr = 0;

    std::function<void(void)> on_remove;

    GuiCollapsingHeader(const char* caption = "CollapsingHeader", bool remove_btn = false, bool enable_background = true, void* user_ptr = 0)
    : caption(caption), user_ptr(user_ptr) {
        setSize(gui::fill(), gui::content());
        setStyleClasses({ "control", "collapsing-header" });

        header.setStyleClasses({ "collapsing-header-header" });
        header.setSize(gui::fill(), gui::em(2));
        header.setOwner(this);
        header.subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
            toggle();
        });
        pushBack(&header);

        content_box.setSize(gui::fill(), gui::content());
        content_box.setStyleClasses({ "collapsing-header-content" });
        content_box.setHidden(true);
        pushBack(&content_box);

        content = &content_box;

        setOpen(false);
    }

    void setOpen(bool value) {
        is_open = value;
        if (is_open) {
            header.clearChildren();
            header.pushBack(ICON_FK_CARET_DOWN, { "icon" })
                ->setSize(gui::em(1.5f), gui::content());
            header.pushBack(caption)
                ->addFlags(GUI_FLAG_SAME_LINE);
            content_box.setHidden(false);
        } else {
            header.clearChildren();
            header.pushBack(ICON_FK_CARET_RIGHT, { "icon" })
                ->setSize(gui::em(1.5f), gui::content());
            header.pushBack(caption)
                ->addFlags(GUI_FLAG_SAME_LINE);
            content_box.setHidden(true);
        }
    }
    void toggle() {
        setOpen(!is_open);
    }
};

