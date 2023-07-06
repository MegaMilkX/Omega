#pragma once

#include "gui/elements/gui_element.hpp"


class GuiMenuBar;
class GuiRoot : public GuiElement {
    std::unique_ptr<GuiMenuBar> menu_bar;
public:
    GuiMenuBar* createMenuBar();

    GuiHitResult onHitTest(int x, int y) override;

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override;

    void onLayout(const gfxm::rect& rect, uint64_t flags) override;

    void onDraw() override;
};