#pragma once

#include "gui/elements/gui_element.hpp"


class GuiMenuBar;
class GuiRoot : public GuiElement {
    std::unique_ptr<GuiMenuBar> menu_bar;
public:
    GuiMenuBar* createMenuBar();

    GuiHitResult hitTest(int x, int y) override;

    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override;

    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rect, uint64_t flags) override;

    void onDraw() override;

    void onLayout2() override;
    void onDraw2() override;
};