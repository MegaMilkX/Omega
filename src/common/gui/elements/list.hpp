#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/list_toolbar_button.hpp"


class GuiListToolbar : public GuiElement {
public:
    GuiListToolbar(bool with_group_buttons = false) {
        setMinSize(gui::fill(), gui::em(2));
        setStyleClasses({ "list-toolbar" });
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
    std::function<void(void)> on_click;
    std::function<void(void)> on_double_click;

    GuiListItem(const char* caption, int user_id = 0, void* user_ptr = 0) {
        this->user_id = user_id;
        this->user_ptr = user_ptr;
        setSize(gui::fill(), gui::em(1.5f));
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
    GuiTreeItem* selected_group = 0;
    bool group_mode = false;

    void setSelectedItem(GuiListItem* item) {
        if (selected_group) {
            selected_group->setSelected(false);
            selected_group = 0;
        }
        if (selected) {
            selected->setSelected(false);
        }
        selected = item;
        if (selected) {
            selected->setSelected(true);
        }
    }
    void setSelectedGroup(GuiTreeItem* group) {
        if (selected) {
            selected->setSelected(false);
            selected = 0;
        }
        if (selected_group) {
            selected_group->setSelected(false);
        }
        selected_group = group;
        if (selected_group) {
            selected_group->setSelected(true);
        }
    }
public:
    std::function<bool(GuiListItem*)> on_add;
    std::function<void(GuiListItem*)> on_remove;
    std::function<bool(GuiTreeItem*)> on_add_group;
    std::function<void(GuiTreeItem*)> on_remove_group;
    std::function<bool(GuiTreeItem*, GuiListItem*)> on_add_to_group;
    std::function<void(GuiTreeItem*, GuiListItem*)> on_remove_from_group;

    GuiList(bool with_groups = false)
    : group_mode(with_groups) {
        setMinSize(gui::fill(), gui::content());

        setStyleClasses({ "list" });

        pushBack(new GuiListToolbar(with_groups), GUI_FLAG_FRAME);
        auto inner = new GuiElement;
        inner->setSize(gui::fill(), gui::content());
        pushBack(inner);
        content = inner;        
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LIST_ADD: {
            if (group_mode) {
                if (childCount() == 0) {
                    if (on_add_group) {
                        GuiTreeItem* group = new GuiTreeItem("Group");
                        if (on_add_group(group)) {
                            pushBack(group);
                            setSelectedGroup(group);
                        } else {
                            delete group;
                            return true;
                        }
                    } else {
                        assert(false);
                        // TODO: !!
                        return true;
                    }
                }
                GuiTreeItem* group = selected_group;
                if (selected_group == 0) {
                    assert(false);
                    LOG_ERR("No group selected");
                    return true;
                }
                if (on_add_to_group) {
                    GuiListItem* item = new GuiListItem("ListItem", 0);
                    if (on_add_to_group(group, item)) {
                        group->pushBack(item);
                        setSelectedItem(item);
                    } else {
                        delete item;
                    }
                }
            } else {
                if (on_add) {
                    GuiListItem* item = new GuiListItem("ListItem", 0);
                    if (on_add(item)) {
                        pushBack(item);
                        setSelectedItem(item);
                    } else {
                        delete item;
                    }
                }
            }
            return true;
        }
        case GUI_MSG::LIST_REMOVE: {
            if (group_mode) {
                if (on_remove_from_group && selected) {
                    auto group = (GuiTreeItem*)selected->getParent();
                    on_remove_from_group(group, selected);
                    group->removeChild(selected);
                    selected = 0;
                }
            } else {
                if (on_remove && selected) {
                    on_remove(selected);
                    removeChild(selected);
                    selected = 0;
                }
            }
            return true;
        }
        case GUI_MSG::LIST_ADD_GROUP: {
            if (on_add_group) {
                GuiTreeItem* group = new GuiTreeItem("Group");
                if (on_add_group(group)) {
                    pushBack(group);
                    setSelectedGroup(group);
                } else {
                    delete group;
                }
            }
            return true;
        }
        case GUI_MSG::LIST_REMOVE_GROUP: {
            if (on_remove_group && selected_group) {
                on_remove_group(selected_group);
                removeChild(selected_group);
                selected_group = 0;
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
                setSelectedItem(params.getB<GuiListItem*>());
                return true;
            case GUI_NOTIFY::TREE_ITEM_CLICK:
                setSelectedGroup(params.getB<GuiTreeItem*>());
                return true;
            }
            break;
        }
        }
        return GuiElement::onMessage(msg, params);
    }

};
