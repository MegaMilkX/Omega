#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/text_element.hpp"
#include "gui/elements/icon.hpp"

class GuiTreeItem : public GuiElement {
    GuiElement* head = 0;
    GuiIconElement* icon = 0;
    GuiTextElement* head_text = 0;
    GuiElement* content_box = 0;

    gfxm::rect rc_header;
    gfxm::rect rc_children;

    bool collapsed = true;
public:
    std::function<void(GuiTreeItem*)> on_click;
    std::string user_string;

    GuiTreeItem(const char* cap = "TreeItem") {
        setSize(gui::fill(), gui::content());

        setStyleClasses({ "tree-item" });
        
        {
            icon = new GuiIconElement;
            icon->setIcon(guiLoadIcon("svg/entypo/triangle-right.svg"));
            icon->setSize(gui::em(1), gui::em(1));
            icon->setHidden(true);

            head_text = new GuiTextElement;
            head_text->setContent(cap);
            head_text->addFlags(GUI_FLAG_SAME_LINE);
            head_text->setReadOnly(true);

            head = new GuiElement;
            head->setSize(gui::fill(), gui::content());
            head->addStyleComponent(gui::style_color{ GUI_COL_TEXT });
            head->_addChild(icon);
            head->_addChild(head_text);
            head->setStyleClasses({ "tree-item-head" });
        }

        content_box = new GuiElement;
        content_box->setSize(gui::fill(), gui::content());
        content_box->setStyleClasses({ "tree-item-content" });
        content_box->overflow = GUI_OVERFLOW_FIT;
        this->content = content_box;
        _addChild(head);
        _addChild(content_box);

        setCollapsed(true);
    }

    void setCollapsed(bool state) {
        collapsed = state;
        content_box->setHidden(collapsed);
        if (collapsed) {
            icon->setIcon(guiLoadIcon("svg/entypo/triangle-right.svg"));
        } else {
            icon->setIcon(guiLoadIcon("svg/entypo/triangle-down.svg"));
        }
    }
    void toggleCollapsed() {
        setCollapsed(!collapsed);
    }
    void setCaption(const char* caption) {
        head_text->setContent(caption);
    }
    void setSelected(bool value) {
        head->setSelected(value);
    }
    GuiTreeItem* addItem(const char* name) {
        auto item = new GuiTreeItem(name);
        addChild(item);
        return item;
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CHILD_ADDED:
        case GUI_MSG::CHILD_REMOVED: {
            if (content->childCount() == 0) {
                icon->setHidden(true);
            } else {
                icon->setHidden(false);
            }
            return true;
        }
        case GUI_MSG::LCLICK:
            notifyOwner<GuiTreeItem*>(GUI_NOTIFY::TREE_ITEM_CLICK, this);
            if (on_click) {
                on_click(this);
            }
            return true;
        case GUI_MSG::DBL_LCLICK:
            toggleCollapsed();
            return true;
        }
        return false;
    }
};
