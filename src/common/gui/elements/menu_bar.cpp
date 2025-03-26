#include "menu_bar.hpp"


#include "gui/gui_system.hpp"
#include "gui/gui.hpp"

GuiMenuItem::GuiMenuItem(const char* cap) {
    caption.replaceAll(getFont(), cap, strlen(cap));
}
GuiMenuItem::GuiMenuItem(const char* caption, const std::initializer_list<GuiMenuListItem*>& child_items)
{
    this->caption.replaceAll(getFont(), caption, strlen(caption));
    menu_list.reset(new GuiMenuList);
    menu_list->setOwner(this);
    menu_list->setHidden(true);
    menu_list->addFlags(GUI_FLAG_MENU_SKIP_OWNER_CLICK);
    guiGetRoot()->addChild(menu_list.get());
    for (auto ch : child_items) {
        menu_list->addItem(ch);
    }
}

void GuiMenuItem::open() {
    menu_list->open();
    gfxm::rect rc = getBoundingRect();
    menu_list->setPosition(rc.min.x, rc.min.y + (rc.max.y - rc.min.y));
    menu_list->setSize(gui::px(200), gui::content());
    is_open = true;
}
void GuiMenuItem::close() {
    menu_list->close();
    is_open = false;
}

bool GuiMenuItem::onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
    switch (msg) {
    case GUI_MSG::CLOSE_MENU:
        if (getOwner()) {
            getOwner()->sendMessage(GUI_MSG::CLOSE_MENU,0,0,0);
        }
        close();
        return true;
    case GUI_MSG::MOUSE_ENTER:
        notifyOwner(GUI_NOTIFY::MENU_ITEM_HOVER, id);
        return true;
    case GUI_MSG::LCLICK:
    case GUI_MSG::DBL_LCLICK:
        notifyOwner(GUI_NOTIFY::MENU_ITEM_CLICKED, id);
        return true;
    case GUI_MSG::NOTIFY:
        switch (params.getA<GUI_NOTIFY>()) {
        case GUI_NOTIFY::MENU_COMMAND:
            close();
            forwardMessageToOwner(msg, params);
            return true;
        }
        break;
    }

    return GuiElement::onMessage(msg, params);
}