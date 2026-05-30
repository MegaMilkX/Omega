#pragma once

#include "gui/elements/menu_list.hpp"


class GuiComboBoxCtrl : public GuiTextElement {
    bool is_open = false;
    std::unique_ptr<GuiMenuList> menu_list;
public:
    GuiComboBoxCtrl(const char* text = "ComboBox") {
        setStyleClasses({ "input-box" });
        GuiTextElement::setContent(text);
        
        menu_list.reset(new GuiMenuList());
        guiGetRoot()->addChild(menu_list.get());
        menu_list->setOwner(this);
        menu_list->addItem(new GuiMenuListItem("Item 1"));
        menu_list->addItem(new GuiMenuListItem("Item 2"));
        menu_list->addItem(new GuiMenuListItem("Item 3"));
        menu_list->addItem(new GuiMenuListItem("Item 4"));
        menu_list->setHidden(true);
        menu_list->addFlags(GUI_FLAG_MENU_SKIP_OWNER_CLICK);

        subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick&) {
            if (!is_open) {
                is_open = true;
                menu_list->open();

                gfxm::vec2 pos = guiConvertPosition(this, guiGetRoot()->getPopupLayer(), gfxm::vec2(rc_bounds.min.x, rc_bounds.max.y));
                menu_list->pos = gui_vec2(pos.x, pos.y);
                menu_list->min_size = gui_vec2(rc_bounds.max.x - rc_bounds.min.x, 0);
                menu_list->max_size = gui_vec2(rc_bounds.max.x - rc_bounds.min.x, 0);
            } else {
                is_open = false;
                menu_list->close();
            }
        });
        content = menu_list.get();
    }

    GuiMenuList* getMenuList() { return menu_list.get(); }

    void setValue(const char* val) {
        content = this;
        clearChildren();
        GuiTextElement::setContent(val);
        content = menu_list.get();
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLOSE_MENU: {
            is_open = false;
            menu_list->close();
            return GuiTextElement::onMessage(msg, params);
        }
        case GUI_MSG::NOTIFY: {
            switch (params.getB<GUI_NOTIFY>()) {
            case GUI_NOTIFY::MENU_COMMAND:
                is_open = false;
                menu_list->close();
                return true;
            }
            break;
        }
        }

        return GuiTextElement::onMessage(msg, params);
    }
};

class GuiComboBox : public GuiElement {
    GuiTextElement label;
    GuiComboBoxCtrl ctrl;
public:
    GuiComboBox(const char* caption = "ComboBox", const char* text = "Select an item...")
        : label(caption), ctrl(text) {
        setSize(gui::fill(), gui::content());
        setStyleClasses({ "control", "container"});

        pushBack(&label);
        label.setSize(gui::perc(25), gui::em(2));
        label.setStyleClasses({ "label" });
        pushBack(&ctrl);
        ctrl.addFlags(GUI_FLAG_SAME_LINE);
        ctrl.setSize(gui::fill(), gui::em(2));

        content = ctrl.getMenuList();
    }

    void setValue(const char* val) {
        ctrl.setValue(val);
    }
};
