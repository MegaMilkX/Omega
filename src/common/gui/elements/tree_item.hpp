#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/text_element.hpp"
#include "gui/elements/icon.hpp"
#include "gui/gui_system.hpp"
#include "IconsForkAwesome.h"


class GuiTreeItem : public GuiElement {
    GuiElement* head = 0;
    GuiIconElement* icon = 0; // arrow
    GuiTextElement* icon2 = nullptr;
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
        addFlags(GUI_FLAG_SELECTABLE);

        setStyleClasses({ "tree-item" });
        
        {
            icon = guiCreate<GuiIconElement>();
            icon->setIcon(guiLoadIcon("svg/entypo/triangle-right.svg"));
            icon->setSize(gui::em(1), gui::em(1));
            icon->setHidden(true);
            icon->addFlags(GUI_FLAG_NO_HIT);

            icon2 = guiCreate<GuiTextElement>();
            icon2->setStyleClasses({"icon"});
            icon2->setContent(ICON_FK_BOOK " ");

            head_text = guiCreate<GuiTextElement>();
            head_text->setContent(cap);
            head_text->addFlags(GUI_FLAG_NO_HIT);
            head_text->setReadOnly(true);

            head = guiCreate<GuiElement>();
            head->setSize(gui::fill(), gui::content());
            head->addStyleComponent(gui::style_color{ GUI_COL_TEXT });
            head->_addChild(icon);
            head->_addChild(icon2);
            head->_addChild(head_text);
            head->setStyleClasses({ "tree-item-head" });
            head->clip_content = false;
            head->primary_axis = GUI_PRIMARY_AXIS::X;
        }

        content_box = guiCreate<GuiElement>();
        content_box->setSize(gui::fill(), gui::content());
        content_box->setStyleClasses({ "tree-item-content" });
        this->content = content_box;
        _addChild(head);
        _addChild(content_box);

        setCollapsed(true);
        
        head->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
            if (e.is_double) {
                toggleCollapsed();
            } else {
                if (on_click) {
                    on_click(this);
                }
            }
        });
        subscribe<GuiEvt_Selected>([this](const GuiEvt_Selected& e) {
            e.consume = false;
            if (e.elem != this) {
                return;
            }
            head->addFlags(GUI_FLAG_SELECTED);
            //LOG_DBG("Selected");
        });
        subscribe<GuiEvt_Deselected>([this](const GuiEvt_Deselected& e) {
            e.consume = false;
            if (e.elem != this) {
                return;
            }
            head->removeFlags(GUI_FLAG_SELECTED);
            //LOG_DBG("Deselected");
        });
    }

    GuiElement* getHead() const { return head; }

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
        auto item = guiCreate<GuiTreeItem>(name);
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
        }
        return false;
    }
};
