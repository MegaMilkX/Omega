#pragma once

#include "gui/elements/text_element.hpp"
#include "gui/gui_system.hpp"
#include "IconsForkAwesome.h"


class GuiCheckbox : public GuiElement {
    GuiTextElement* box = nullptr;
    bool value = false;
public:
    GuiCheckbox(const std::string& caption = "Checkbox") {
        setSize(gui::fill(), gui::content());
        setStyleClasses({ "control", "container" });
        primary_axis = GUI_PRIMARY_AXIS::X;

        GuiTextElement* label = pushBack(guiCreate<GuiTextElement>(caption));
        label->setReadOnly(true);
        label->setSize(gui::perc(25), gui::em(1.70));
        label->setStyleClasses({"label"});

        box = pushBack(guiCreate<GuiTextElement>());
        box->setSize(gui::em(1.70), gui::em(1.70));
        box->setReadOnly(true);
        box->setStyleClasses({ "input-box", "icon" });
        box->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick&) {
            value = !value;
            setValue(value);
            invoke(GuiEvt_Changed{});
        });
    }

    void setValue(bool v) {
        value = v;
        if (value) {
            box->setContent(ICON_FK_CHECK);
        } else {
            box->setContent("");
        }
    }
    bool getValue() const {
        return value;
    }
};

