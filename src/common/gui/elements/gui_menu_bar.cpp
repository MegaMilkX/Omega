#include "gui_menu_bar.hpp"


#include "gui/gui_system.hpp"
#include "gui/gui.hpp"

GuiMenuItem::GuiMenuItem(const char* cap)
    : caption(guiGetDefaultFont()) {
    caption.replaceAll(cap, strlen(cap));
    menu_list.reset(new GuiMenuList);
}
void GuiMenuItem::onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
    switch (msg) {
    case GUI_MSG::CLICKED:
    case GUI_MSG::DBL_CLICKED:
        if (!is_open) {
            menu_list.reset(new GuiMenuList());
            menu_list->setOwner(this);
            menu_list->pos = gfxm::vec2(bounding_rect.min.x, bounding_rect.max.y);
            menu_list->size = gfxm::vec2(
                bounding_rect.max.x - bounding_rect.min.x,
                100.0f
            );
            guiGetRoot()->addChild(menu_list.get());
            guiSetFocusedWindow(menu_list.get());
            is_open = true;
        } else {
            //guiGetRoot()->removeChild(menu_list.get());
            //menu_list.reset(0);
            //is_open = false;
        }
        break;
    case GUI_MSG::NOTIFY:
        switch (params.getA<GUI_NOTIFICATION>()) {
        case GUI_NOTIFICATION::MENU_LIST_UNFOCUS:
            if (is_open) {
                guiGetRoot()->removeChild(menu_list.get());
                menu_list.reset(0);
                is_open = false;
            }
            break;
        }
        break;
    }

    GuiElement::onMessage(msg, params);
}