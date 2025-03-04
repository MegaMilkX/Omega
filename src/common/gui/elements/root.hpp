#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/menu_bar.hpp"
#include "gui/elements/title_bar.hpp"


class GuiMenuBar;
class GuiRoot : public GuiElement {
    std::unique_ptr<GuiMenuBar> menu_bar;
public:
    GuiRoot() {
        //box.setSize(0, 0);
        setStyleClasses({ "root" });
        addFlags(GUI_FLAG_NO_HIT);
        overflow = GUI_OVERFLOW_NONE;

        /*
        auto title_bar = new GuiTitleBar();
        title_bar->addFlags(GUI_FLAG_PERSISTENT | GUI_FLAG_FRAME);
        pushBack(title_bar);*/
    }

    GuiMenuBar* createMenuBar();

    //void onHitTest(GuiHitResult& hit, int x, int y) override;

    //void onLayout(const gfxm::rect& rect, uint64_t flags) override;

    //void onDraw() override;
};