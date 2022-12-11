#include "gui_menu_bar.hpp"


#include "gui/gui_system.hpp"
#include "gui/gui.hpp"

GuiMenuItem::GuiMenuItem(const char* cap)
    : caption(guiGetDefaultFont()) {
    caption.replaceAll(cap, strlen(cap));/*
    menu_list.reset(new GuiMenuList);
    menu_list->setOwner(this);
    guiGetRoot()->addChild(menu_list.get());
    menu_list->is_hidden = true;*/
}
GuiMenuItem::GuiMenuItem(const char* caption, const std::initializer_list<GuiMenuListItem*>& child_items)
: caption(guiGetDefaultFont()) {
    this->caption.replaceAll(caption, strlen(caption));
    menu_list.reset(new GuiMenuList);
    menu_list->setOwner(this);
    menu_list->is_hidden = true;
    guiGetRoot()->addChild(menu_list.get());
    for (auto ch : child_items) {
        menu_list->addItem(ch);
    }
}

void GuiMenuItem::open() {
    menu_list->open();
    menu_list->pos = client_area.min + gfxm::vec2(.0f, client_area.max.y - client_area.min.y);
    menu_list->size = gfxm::vec2(200, 200);
    is_open = true;
}
void GuiMenuItem::close() {
    menu_list->close();
    is_open = false;
}

void GuiMenuItem::onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
    switch (msg) {
    case GUI_MSG::MOUSE_ENTER:
        notifyOwner(GUI_NOTIFY::MENU_ITEM_HOVER, id);
        break;
    case GUI_MSG::CLICKED:
    case GUI_MSG::DBL_CLICKED:
        notifyOwner(GUI_NOTIFY::MENU_ITEM_CLICKED, id);
        break;
    case GUI_MSG::NOTIFY:
        switch (params.getA<GUI_NOTIFY>()) {
        case GUI_NOTIFY::MENU_COMMAND:
            close();
            forwardMessageToOwner(msg, params);
            break;
        }
        break;
    }

    GuiElement::onMessage(msg, params);
}