#pragma once

#include <functional>
#include "gui/elements/element.hpp"
#include "gui/elements/label.hpp"
#include "gui/elements/input_string_box.hpp"

class GuiInputString : public GuiElement {
protected:
    GuiInputStringBox* box = 0;

public:
    GuiInputString(
        const char* caption = "InputString"
    ) {
        setSize(gui::fill(), gui::content());
        setStyleClasses({ "control", "container" });

        GuiTextElement* label = pushBack(new GuiTextElement(caption));
        label->setReadOnly(true);
        label->setSize(gui::perc(25), gui::em(2));
        label->setStyleClasses({"label"});

        box = pushBack(new GuiInputStringBox());
        box->setSize(gui::fill(), gui::em(2));
        box->addFlags(GUI_FLAG_SAME_LINE);
    }

    std::function<void(const std::string&)> on_change;

    void setValue(const std::string& value) {
        if(value != box->getValue()) {
            box->setValue(value);
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NUMERIC_UPDATE: {
            if (on_change) {
                on_change(box->getValue());
            }
            return true;
        }
        }
        return GuiElement::onMessage(msg, params);
    }
};