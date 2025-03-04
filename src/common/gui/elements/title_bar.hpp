#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/text_element.hpp"
#include "gui/elements/button.hpp"
#include "gui/gui_icon.hpp"

extern GuiIcon* guiLoadIcon(const char* svg_path);

class GuiTitleBar : public GuiElement {
public:
    GuiTitleBar() {
        setSize(gui::perc(100), gui::em(2));
        setMinSize(gui::perc(100), gui::em(2));
        setMaxSize(gui::perc(100), gui::em(2));

        setStyleClasses({ "title-bar" });

        //auto btn_close = new GuiButton("", guiLoadIcon("svg/Entypo/cross.svg"));
        //pushBack(btn_close);

        auto caption = new GuiTextElement("MyCoolEngine");
        caption->setReadOnly(true);
        caption->setSize(gui::perc(100), gui::perc(100));
        caption->setMinSize(gui::perc(100), gui::perc(100));
        caption->setMaxSize(gui::perc(100), gui::perc(100));
        caption->addFlags(GUI_FLAG_SAME_LINE);
        
        pushBack(caption);
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(rc_bounds, gfxm::vec2(x, y))) {
            return;
        }

        hit.add(GUI_HIT::NATIVE_CAPTION, this);
    }
};