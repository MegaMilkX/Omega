#pragma once

#include "gui/elements/element.hpp"


class GuiMenuBar;
class GuiRoot : public GuiElement {
    std::unique_ptr<GuiMenuBar> menu_bar;
public:
    GuiMenuBar* createMenuBar();

    //void onHitTest(GuiHitResult& hit, int x, int y) override;

    //void onLayout(const gfxm::rect& rect, uint64_t flags) override;

    //void onDraw() override;
};