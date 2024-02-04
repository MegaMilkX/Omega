#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/list_toolbar_button.hpp"


class GuiListToolbar : public GuiElement {
public:
    GuiListToolbar(bool with_group_buttons = false) {
        setMinSize(0, gui::em(2));
        setStyleClasses({ "list-toolbar" });
        overflow = GUI_OVERFLOW_FIT;
        if (with_group_buttons) {
            pushBack(
                new GuiListToolbarButton(guiLoadIcon("svg/entypo/circle-with-plus.svg"), GUI_MSG::LIST_ADD_GROUP),
                GUI_FLAG_SAME_LINE
            );
            pushBack(
                new GuiListToolbarButton(guiLoadIcon("svg/entypo/circle-with-minus.svg"), GUI_MSG::LIST_REMOVE_GROUP),
                GUI_FLAG_SAME_LINE
            );
        }
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
    void* user_ptr = 0;
    std::function<void(void)> on_click;
    std::function<void(void)> on_double_click;

    GuiListItem(const char* caption, void* user_ptr)
    : user_ptr(user_ptr) {
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
            notifyOwner(GUI_NOTIFY::LIST_ITEM_SELECTED, this);
            if (on_click) {
                on_click();
            }
            } return true;
        }

        return GuiElement::onMessage(msg, params);
    }
};

class GuiList : public GuiElement {
    GuiListItem* selected = 0;
    bool group_mode = false;
public:
    std::function<void(void)> on_add;
    std::function<void(GuiListItem*)> on_remove;
    std::function<void(void)> on_add_group;
    std::function<void(GuiTreeItem*)> on_remove_group;
    std::function<void(GuiTreeItem*)> on_add_to_group;
    std::function<void(GuiTreeItem*, GuiListItem*)> on_remove_from_group;

    GuiList(bool with_groups = false)
    : group_mode(with_groups) {
        overflow = GUI_OVERFLOW_FIT;
        setMinSize(0, gui::em(4));

        setStyleClasses({ "list" });

        pushBack(new GuiListToolbar(with_groups), GUI_FLAG_FRAME);
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LIST_ADD: {
            if (group_mode) {
                if (childCount() == 0) {
                    auto group = new GuiTreeItem("Group");
                    pushBack(group);
                }
                GuiTreeItem* group = 0;
                if (selected == 0) {
                    group = (GuiTreeItem*)getChild(childCount() - 1);
                } else {
                    group = (GuiTreeItem*)selected->getParent();
                }
                if (on_add_to_group) {
                    on_add_to_group(group);
                }
            } else {
                if (on_add) {
                    on_add();
                }
            }
            return true;
        }
        case GUI_MSG::LIST_REMOVE: {
            if (group_mode) {
                if (on_remove_from_group && selected) {
                    on_remove_from_group((GuiTreeItem*)selected->getParent(), selected);
                }
            } else {
                if (on_remove && selected) {
                    on_remove(selected);
                }
            }
            return true;
        }
        case GUI_MSG::LIST_ADD_GROUP: {
            if (on_add_group) {
                on_add_group();
            }
            return true;
        }
        case GUI_MSG::LIST_REMOVE_GROUP: {
            if (on_remove_group && selected) {
                auto group = (GuiTreeItem*)selected->getParent();
                on_remove_group(group);
            }
            return true;
        }
        case GUI_MSG::CHILD_REMOVED: {
            if (selected == params.getA<GuiElement*>()) {
                selected = 0;
            }
            return true;
        }
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::LIST_ITEM_SELECTED:
                if (selected) {
                    selected->setSelected(false);
                }
                selected = params.getB<GuiListItem*>();
                selected->setSelected(true);
                return true;
            }
            break;
        }
        }
        return GuiElement::onMessage(msg, params);
    }

};
