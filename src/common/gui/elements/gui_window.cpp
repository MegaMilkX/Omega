#include "gui/elements/gui_window.hpp"

GuiWindow::GuiWindow(const char* title_str)
: title(title_str), title_buf(guiGetDefaultFont()) {
    close_btn.setOwner(this);

    title_buf.replaceAll(title_str, strlen(title_str));
    setMinSize(150, 100);
    setSize(640, 480);
    content_padding = gfxm::rect(5, 5, 5, 5);

    scroll_v.reset(new GuiScrollBarV());
    scroll_v->setOwner(this);
    scroll_h.reset(new GuiScrollBarH());
    scroll_h->setOwner(this);

    setFlags(GUI_FLAG_OVERLAPPED);

    guiSetActiveWindow(this);
}
GuiWindow::~GuiWindow() {
    if (guiGetActiveWindow() == this) {
        guiSetActiveWindow(0);
    }
}
