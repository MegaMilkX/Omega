#pragma once

#include "gui/elements/menu_list.hpp"
#include "IconsForkAwesome.h"


class GuiComboBoxCtrl : public GuiElement {
    bool is_open = false;
    std::unique_ptr<GuiMenuList> menu_list;
    GuiTextElement* caption = nullptr;
    GuiTextElement* icon = nullptr;

    void setOpen(bool state) {
        is_open = state;
        if (!is_open) {
            guiRemoveTransientScope(this);
            menu_list->close();
        } else {
            guiAddTransientScope(this, nullptr, GUI_TRANSIENT_SCOPE_POP);
            menu_list->open();
        }
    }
public:
    GuiComboBoxCtrl(const char* text = "ComboBox") {
        setStyleClasses({ "combo-box-ctrl" });

        primary_axis = GUI_PRIMARY_AXIS::X;

        caption = pushBack(guiCreate<GuiTextElement>(text));
        caption->setSize(gui::fill(), gui::content());
        caption->addFlags(GUI_FLAG_NO_HIT);
        icon = pushBack(guiCreate<GuiTextElement>());
        icon->setSize(gui::content(), gui::content());
        icon->setStyleClasses({ "icon" });
        icon->setContent(ICON_FK_CARET_DOWN);
        icon->addFlags(GUI_FLAG_NO_HIT);

        menu_list.reset(new GuiMenuList());
        menu_list->setOwner(this);
        menu_list->setHidden(true);
        menu_list->addFlags(GUI_FLAG_MENU_SKIP_OWNER_CLICK);

        subscribe<GuiEvt_ScopeLeft>([this](const GuiEvt_ScopeLeft& e) {
            LOG_DBG("COMBO BOX CTRL: SCOPE OUTSIDE");
            setOpen(false);
        });

        subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick&) {
            if (!is_open) {
                setOpen(true);

                gfxm::vec2 pos = guiConvertPosition(this, guiGetRoot()->getPopupLayer(), gfxm::vec2(rc_bounds.min.x, rc_bounds.max.y));
                menu_list->pos = gui_vec2(pos.x, pos.y);
                menu_list->size = gui_vec2(rc_bounds.max.x - rc_bounds.min.x, gui::content());
                //menu_list->min_size = gui_vec2(rc_bounds.max.x - rc_bounds.min.x, 0);
                //menu_list->max_size = gui_vec2(rc_bounds.max.x - rc_bounds.min.x, 0);
            } else {
                setOpen(false);
            }
        });
        subscribe<GuiEvt_MenuCmd>([this](const GuiEvt_MenuCmd& e) {
            setOpen(false);
            e.consume = false;
        });
        content = menu_list.get();
    }

    GuiMenuList* getMenuList() { return menu_list.get(); }

    void setValue(const char* val) {
        caption->setContent(val);
    }

    GuiMenuListItem* addItem(const std::string& label, int cmd) {
        return menu_list->addItem(label, cmd);
    }
};

class GuiComboBox : public GuiElement {
    GuiTextElement label;
    GuiComboBoxCtrl ctrl;

    struct InternalItem {
        std::string label;
    };
    std::unordered_map<int, InternalItem> items;
    int chosen = -1;
public:
    GuiComboBox(const char* caption = "ComboBox", const char* text = "...")
        : label(caption), ctrl(text) {
        setSize(gui::fill(), gui::content());
        setStyleClasses({ "control", "container"});
        primary_axis = GUI_PRIMARY_AXIS::X;

        pushBack(&label);
        label.setSize(gui::perc(25), gui::em(1.70));
        label.setStyleClasses({ "label" });
        pushBack(&ctrl);
        ctrl.setSize(gui::fill(), gui::em(1.70));

        content = ctrl.getMenuList();
        
        ctrl.subscribe<GuiEvt_MenuCmd>([this](const GuiEvt_MenuCmd& e) {
            e.invoke_next();
            ctrl.setValue(items[e.id].label.c_str());
            chosen = e.id;
            invoke(GuiEvt_Changed{});
        });
    }

    void setValue(const char* val) {
        // TODO: Remove
        ctrl.setValue(val);
    }

    void setCurrent(int id) {
        ctrl.setValue(items[id].label.c_str());
        chosen = id;
    }
    int getCurrent() const {
        return chosen;
    }

    GuiMenuListItem* addItem(const std::string& label, int cmd) {
        if (items.empty()) {
            ctrl.setValue(label.c_str());
            chosen = 0;
        }
        items[cmd] = InternalItem{ label };
        return ctrl.addItem(label, cmd);
    }
};
