#include "menu_bar.hpp"


#include "gui/gui_system.hpp"
#include "gui/gui.hpp"

GuiMenuItem::GuiMenuItem(const char* cap) {
    setSize(gui::content(), gui::fill());
    setStyleClasses({ "menu-item" });
    pushBack(cap)->addFlags(GUI_FLAG_NO_HIT);

    _setEventHandlers();
}
GuiMenuItem::GuiMenuItem(const char* caption, const std::initializer_list<GuiMenuListItem*>& child_items) {
    setSize(gui::content(), gui::fill());
    setStyleClasses({ "menu-item" });
    pushBack(caption)->addFlags(GUI_FLAG_NO_HIT);

    menu_list.reset(new GuiMenuList);
    menu_list->setOwner(this);
    menu_list->setHidden(true);
    //menu_list->addFlags(GUI_FLAG_MENU_SKIP_OWNER_CLICK);
    guiGetRoot()->addChild(menu_list.get());
    for (auto ch : child_items) {
        menu_list->addItem(ch);
    }

    _setEventHandlers();
}

void GuiMenuItem::open() {
    if (is_open) {
        return;
    }
    menu_list->open();
    gfxm::rect rc = getBoundingRect();
    gfxm::vec2 pos(rc.min.x, rc.min.y + (rc.max.y - rc.min.y));
    pos = guiConvertPosition(this, guiGetRoot()->getPopupLayer(), pos);
    menu_list->setPosition(pos.x, pos.y);
    menu_list->setSize(gui::px(200), gui::content());
    is_open = true;
    
    guiAddTransientScope(this, GUI_TRANSIENT_SCOPE_POP);
}
void GuiMenuItem::close() {
    if (!is_open) {
        return;
    }
    guiRemoveTransientScope(this);

    if(!menu_list) return;
    menu_list->close();
    is_open = false;
}
void GuiMenuItem::toggle() {
    if (is_open) {
        close();
    } else {
        open();
    }
}

