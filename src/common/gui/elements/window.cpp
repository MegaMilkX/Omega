#include "gui/elements/window.hpp"

GuiWindow::GuiWindow(const char* title_str)
: title(title_str) {
    close_btn.reset(new GuiWindowTitleBarButton(guiLoadIcon("svg/entypo/cross.svg"), GUI_MSG::CLOSE));
    close_btn->setOwner(this);

    title_buf.replaceAll(getFont(), title_str, strlen(title_str));
    setMinSize(150, 100);
    setSize(640, 480);
    setStyleClasses({ "window" });

    scroll_v.reset(new GuiScrollBarV());
    scroll_v->setOwner(this);
    scroll_h.reset(new GuiScrollBarH());
    scroll_h->setOwner(this);

    setFlags(GUI_FLAG_WINDOW | GUI_FLAG_FLOATING);

    guiSetActiveWindow(this);
}
GuiWindow::~GuiWindow() {
    if (guiGetActiveWindow() == this) {
        guiSetActiveWindow(0);
    }
}
