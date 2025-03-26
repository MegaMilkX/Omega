#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/menu_bar.hpp"
#include "gui/elements/title_bar.hpp"


class GuiMenuBar;
class GuiRoot : public GuiElement {
    GuiElement* menu_box = 0;
    GuiElement* content_box = 0;

    std::unique_ptr<GuiMenuBar> menu_bar;
public:
    GuiRoot() {
        setStyleClasses({ "root" });
        addFlags(GUI_FLAG_NO_HIT);

        menu_box = new GuiElement();
        menu_box->setSize(gui::fill(), gui::content());
        pushBack(menu_box);

        content_box = new GuiElement();
        content_box->setSize(gui::fill(), gui::fill());
        content_box->addFlags(GUI_FLAG_NO_HIT);
        pushBack(content_box);

        content = content_box;

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