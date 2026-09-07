#pragma once

#include "gui/gui_system.hpp"
#include "gui/elements/element.hpp"
#include "gui/gui_icon.hpp"


class GuiMenuList;
class GuiMenuListItem : public GuiElement {
    GuiTextBuffer caption;
    bool is_open = false;
    std::unique_ptr<GuiMenuList> menu_list;
    GuiIcon* icon_arrow = 0;

    void _setEventHandlers() {
        subscribe<GuiEvt_ScopeLeft>([this](const GuiEvt_ScopeLeft& e) {
            LOG_DBG("MENU LIST ITEM: SCOPE OUTSIDE");
            close();
        });
        subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick&) {
            if (hasList()) {
                open();
            } else {
                invokeBubble(GuiEvt_MenuCmd(command_identifier));

                notifyOwner(GUI_NOTIFY::MENU_COMMAND, command_identifier);
                if (on_click) {
                    on_click();
                }
            }
        });
        subscribe<GuiEvt_MouseEnter>([this](const GuiEvt_MouseEnter&) {
            notifyOwner(GUI_NOTIFY::MENU_ITEM_HOVER, id);
        });
    }
public:
    std::function<void(void)> on_click;

    int id = 0;
    int command_identifier = 0;
    void* user_ptr = 0;

    void open();
    void close();

    bool hasList() { return menu_list.get() != nullptr; }

    GuiMenuListItem(const char* cap, std::function<void(void)> on_click)
        : on_click(on_click) {
        setSize(gui::fill(), gui::em(2));
        caption.replaceAll(getFont(), cap, strlen(cap));
        icon_arrow = guiLoadIcon("svg/entypo/triangle-right.svg");

        _setEventHandlers();
    }
    GuiMenuListItem(const char* cap = "MenuListItem", int cmd = 0)
        : command_identifier(cmd) {
        setSize(gui::fill(), gui::em(2));
        caption.replaceAll(getFont(), cap, strlen(cap));
        icon_arrow = guiLoadIcon("svg/entypo/triangle-right.svg");

        _setEventHandlers();
    }
    GuiMenuListItem(const char* cap, const std::initializer_list<GuiMenuListItem*>& child_items);
    
    void onDraw() override {
        Font* font = getFont();
        if (isHovered()) {
            guiDrawRect(client_area, GUI_COL_BUTTON);
        }
        {
            gfxm::rect rc = client_area;
            rc.min.x += GUI_MARGIN;
            caption.draw(font, rc, GUI_LEFT | GUI_VCENTER, GUI_COL_TEXT, GUI_COL_ACCENT);
            //caption.draw(client_area.min + gfxm::vec2(GUI_MARGIN, guiGetCurrentFont()->font->getLineHeight() * .25f), GUI_COL_TEXT, GUI_COL_ACCENT);
        }
        float fontH = font->getLineHeight();
        if (hasList() && icon_arrow) {
            icon_arrow->draw(guiLayoutPlaceRectInsideRect(client_area, gfxm::vec2(fontH, fontH), GUI_RIGHT | GUI_VCENTER, gfxm::rect(GUI_MARGIN, GUI_MARGIN, GUI_MARGIN, GUI_MARGIN)), GUI_COL_TEXT);
        }
    }
};
class GuiMenuList : public GuiElement {
    std::vector<std::unique_ptr<GuiMenuListItem>> items;
    GuiMenuListItem* open_elem = 0;
public:
    void open() {
        setHidden(false);
        guiGetRoot()->getPopupLayer()->addChild(this);
    }
    void close() {
        setHidden(true);
        if (open_elem) {
            open_elem->close();
        }
        guiGetRoot()->getPopupLayer()->removeChild(this);
    }

    GuiMenuList() {
        setSize(gui::px(300), gui::content());
        addFlags(
            GUI_FLAG_TOPMOST
            //| GUI_FLAG_MENU_POPUP
        );

        subscribe<GuiEvt_MenuCmd>([this](const GuiEvt_MenuCmd& e) {
            if (getOwner()) {
                getOwner()->invokeBubble(e);
            } else if (getParent()) {
                // fallback
                getParent()->invokeBubble(e);
            }
        });
    }
    GuiMenuListItem* addItem(const std::string& label, int cmd) {
        items.push_back(std::unique_ptr<GuiMenuListItem>(new GuiMenuListItem(label.c_str(), cmd)));
        pushBack(items.back().get());
        items.back()->setOwner(this);
        return items.back().get();
    }
    GuiMenuList* addItem(GuiMenuListItem* item) {
        item->id = items.size();
        items.push_back(std::unique_ptr<GuiMenuListItem>(item));
        addChild(item);
        item->setOwner(this);
        return this;
    }
    void onDraw() override {
        guiDrawRectShadow(rc_bounds);
        guiDrawRect(rc_bounds, GUI_COL_HEADER);
        guiDrawRectLine(rc_bounds, GUI_COL_BUTTON);
        GuiElement::onDraw();
        /*
        for (int i = 0; i < childCount(); ++i) {
            auto ch = getChild(i);
            ch->draw();
        }*/
    }
};
inline GuiMenuListItem::GuiMenuListItem(const char* cap, const std::initializer_list<GuiMenuListItem*>& child_items) {
    setSize(gui::fill(), gui::em(2));
    caption.replaceAll(getFont(), cap, strlen(cap));
    menu_list.reset(new GuiMenuList);
    menu_list->setOwner(this);
    menu_list->setHidden(true);
    menu_list->addFlags(GUI_FLAG_MENU_SKIP_OWNER_CLICK);
    guiGetRoot()->addChild(menu_list.get());
    for (auto ch : child_items) {
        menu_list->addItem(ch);
    }
    icon_arrow = guiLoadIcon("svg/entypo/triangle-right.svg");

    _setEventHandlers();
}
inline void GuiMenuListItem::open() {
    if (is_open) {
        return;
    }
    menu_list->open();
    gfxm::vec2 pos = guiConvertPosition(this, guiGetRoot()->getPopupLayer(), gfxm::vec2(client_area.max.x, client_area.min.y));
    menu_list->pos = gui_vec2(pos.x, pos.y);
    menu_list->setSize(gui::px(200), gui::content());
    is_open = true;
    guiBringWindowToTop(menu_list.get());
    guiAddTransientScope(this, nullptr, GUI_TRANSIENT_SCOPE_POP);
}
inline void GuiMenuListItem::close() {
    if (!is_open) {
        return;
    }
    guiRemoveTransientScope(this);
    if(!menu_list) return;
    menu_list->close();
    is_open = false;
}
