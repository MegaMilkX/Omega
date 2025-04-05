#pragma once

#include <functional>
#include "gui/elements/element.hpp"
#include "gui/elements/label.hpp"
#include "gui/elements/icon.hpp"
#include "gui/elements/input_numeric_box.hpp"
#include "gui/elements/input.hpp"
#include "gui/elements/text_element.hpp"
#include "gui/gui_system.hpp"


class GuiInputResource : public GuiElement {
protected:
    GuiTextElement* text = 0;
    GuiIconElement* icon = 0;
    GuiInputStringBox* box = 0;
    GuiButton* btn_browse = 0;
    GuiButton* btn_reload = 0;
    GuiButton* btn_remove = 0;

    GuiElement* left = 0;
    GuiElement* right = 0;

public:
    GuiInputResource(
        const char* caption = "InputResource"
    ) {
        setSize(gui::fill(), gui::content());
        setStyleClasses({ "control", "container" });

        left = new GuiElement;
        left->setStyleClasses({ "container" });
        left->setSize(gui::perc(25), gui::content());
        right = new GuiElement;
        right->setStyleClasses({ "container" });
        right->setSize(gui::fill(), gui::content());
        right->addFlags(GUI_FLAG_SAME_LINE);
        left->setParent(this);
        right->setParent(this);

        text = new GuiTextElement("model");
        text->setReadOnly(true);
        text->setSize(gui::fill(), gui::em(2));
        text->setStyleClasses({"label"});
        icon = new GuiIconElement();
        icon->setIcon(guiLoadIcon("svg/Entypo/archive.svg"));
        box = new GuiInputStringBox();
        box->setSize(gui::fill(), gui::em(2));
        box->setValue("./model.skeletal_model");
        btn_browse = new GuiButton("", guiLoadIcon("svg/Entypo/folder.svg"));
        btn_reload = new GuiButton("", guiLoadIcon("svg/Entypo/ccw.svg"));
        btn_reload->addFlags(GUI_FLAG_SAME_LINE);
        btn_remove = new GuiButton("", guiLoadIcon("svg/Entypo/cross.svg"));
        btn_remove->addFlags(GUI_FLAG_SAME_LINE);

        //left->pushBack(icon);
        left->pushBack(text);
        right->pushBack(box);
        right->pushBack(btn_browse);
        right->pushBack(btn_reload);
        right->pushBack(btn_remove);
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