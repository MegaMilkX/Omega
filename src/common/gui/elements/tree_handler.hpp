#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/tree_item.hpp"

class GuiTreeHandler : public GuiElement {
    GuiTreeItem* selected_item = 0;
public:
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CHILD_REMOVED: {
            auto ch = params.getA<GuiElement*>();
            if (ch == selected_item) {
                selected_item = 0;
            }
            return false;
        }
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {
            case GUI_NOTIFY::TREE_ITEM_CLICK: {
                if (selected_item) {
                    selected_item->setSelected(false);
                }
                selected_item = params.getB<GuiTreeItem*>();
                if (selected_item) {
                    selected_item->setSelected(true);
                }
                return true;
            }
            }
            break;
        }
        }
        return GuiElement::onMessage(msg, params);
    }
};