#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/list_toolbar_button.hpp"


class GuiListDataSource {
public:

};


class GuiListToolbar : public GuiElement {
public:
    GuiListToolbar() {
        setMinSize(0, gui::em(2));
        setStyleClasses({ "list-toolbar" });
        overflow = GUI_OVERFLOW_FIT;
        pushBack(
            new GuiListToolbarButton(guiLoadIcon("svg/entypo/plus.svg"), GUI_MSG::LIST_ADD),
            GUI_FLAG_SAME_LINE
        );
        pushBack(
            new GuiListToolbarButton(guiLoadIcon("svg/entypo/minus.svg"), GUI_MSG::LIST_REMOVE),
            GUI_FLAG_SAME_LINE
        );
    }
};

class GuiListItem : public GuiElement {
public:
    int item_id = 0;
    std::function<void(void)> on_click;
    std::function<void(void)> on_double_click;

    GuiListItem(const char* caption, int id)
    : item_id(id) {
        //overflow = GUI_OVERFLOW_FIT;
        setSize(0, gui::em(1.5f));
        setStyleClasses({ "list-item" });
        pushBack(caption);
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::DBL_LCLICK: {
            if (on_double_click) {
                on_double_click();
            }
            return true;
        }
        case GUI_MSG::LCLICK: {
            if (on_click) {
                on_click();
            }
            } return true;
        }

        return GuiElement::onMessage(msg, params);
    }
};

class GuiList : public GuiElement {
    GuiListDataSource* data_source = 0;
public:
    std::function<void(void)> on_add;
    std::function<void(void)> on_remove;

    GuiList(GuiListDataSource* data_source = 0)
    : data_source(data_source) {
        overflow = GUI_OVERFLOW_FIT;
        setMinSize(0, gui::em(4));

        setStyleClasses({ "list" });

        pushBack(new GuiListToolbar(), GUI_FLAG_FRAME);
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LIST_ADD: {
            if (on_add) {
                on_add();
            }
            return true;
        }
        case GUI_MSG::LIST_REMOVE: {
            if (on_remove) {
                on_remove();
            }
            return true;
        }
        }
        return GuiElement::onMessage(msg, params);
    }

};
