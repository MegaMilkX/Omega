#include "gui/elements/window.hpp"

GuiWindow::GuiWindow(const char* title_str)
: title(title_str) {
    //close_btn.reset(new GuiListToolbarButton(guiLoadIcon("svg/entypo/cross.svg"), GUI_MSG::CLOSE));
    //close_btn->setOwner(this);

    setMinSize(150, 100);
    setSize(640, 480);
    setStyleClasses({ "window" });

    //guiSetActiveWindow(this);
}
GuiWindow::~GuiWindow() {
    /*if (guiGetActiveWindow() == this) {
        guiSetActiveWindow(0);
    }*/
}
