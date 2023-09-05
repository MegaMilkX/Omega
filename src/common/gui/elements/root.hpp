#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/menu_bar.hpp"


class GuiMenuBar;
class GuiRoot : public GuiElement {
    std::unique_ptr<GuiMenuBar> menu_bar;
public:
    GuiRoot() {
        //box.setSize(0, 0);
        setStyleClasses({ "root" });
    }

    GuiMenuBar* createMenuBar();

    //void onHitTest(GuiHitResult& hit, int x, int y) override;

    //void onLayout(const gfxm::rect& rect, uint64_t flags) override;

    //void onDraw() override;
};