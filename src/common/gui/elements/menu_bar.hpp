#pragma once

#include "gui/elements/element.hpp"
#include "gui/gui_font.hpp"
#include "gui/gui_text_buffer.hpp"


class GuiMenuListItem;
class GuiMenuList;
class GuiMenuItem : public GuiElement {
    bool is_open = false;
    std::unique_ptr<GuiMenuList> menu_list;

    void _setEventHandlers() {
        subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick&) {
            if(!menu_list) return;
            toggle();
        });
        subscribe<GuiEvt_MouseEnter>([this](const GuiEvt_MouseEnter&) {
            //notifyOwner(GUI_NOTIFY::MENU_ITEM_HOVER, id);
        });
        subscribe<GuiEvt_ScopeLeft>([this](const GuiEvt_ScopeLeft& e) {
            LOG_DBG("MENU ITEM: SCOPE OUTSIDE");
            close();
        });
    }
public:
    int id = 0;

    GuiMenuItem(const char* caption);
    GuiMenuItem(const char* caption, const std::initializer_list<GuiMenuListItem*>& child_items);

    void open();
    void close();
    void toggle();
    
    bool hasList() { return menu_list.get() != nullptr; }
};

class GuiMenuBar : public GuiElement {
    std::vector<std::unique_ptr<GuiMenuItem>> items;
    bool is_active = false;
    GuiMenuItem* open_elem = 0;
public:
    GuiMenuBar() {
        setSize(gui::fill(), gui::em(2));
        setStyleClasses({ "menu-bar" });
        primary_axis = GUI_PRIMARY_AXIS::X;
    }
    GuiMenuBar* addItem(GuiMenuItem* item) {
        assert(item->getOwner() == nullptr && item->getParent() == nullptr);
        item->id = items.size();
        item->subscribe<GuiEvt_LClick>([this, item](const GuiEvt_LClick& e) {
            e.invoke_next();
            /*if (item->hasList()) {
                if (!is_active) {
                    is_active = true; 
                    open_elem = item;
                    open_elem->open();
                } else {
                    is_active = false;
                    if (open_elem) {
                        open_elem->close();
                        open_elem = 0;
                    }
                }
            }*/
        });
        std::unique_ptr<GuiMenuItem> uptritem(item);
        items.push_back(std::move(uptritem));
        addChild(item);
        item->setOwner(this);
        return this;
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        for (int i = 0; i < childCount(); ++i) {
            auto c = getChild(i);
            c->hitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
};
