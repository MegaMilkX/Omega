#pragma once

#include <functional>
#include "gui/elements/element.hpp"
#include "gui/elements/icon.hpp"
#include "gui/elements/input_numeric_box.hpp"
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
        primary_axis = GUI_PRIMARY_AXIS::X;

        left = new GuiElement;
        left->setStyleClasses({ "container" });
        left->setSize(gui::perc(25), gui::content());
        right = new GuiElement;
        right->setStyleClasses({ "container" });
        right->setSize(gui::fill(), gui::content());
        right->addFlags(GUI_FLAG_SAME_LINE);
        right->primary_axis = GUI_PRIMARY_AXIS::Y;
        left->setParent(this);
        right->setParent(this);

        text = new GuiTextElement("model");
        text->setReadOnly(true);
        text->setSize(gui::fill(), gui::em(1.70));
        text->setStyleClasses({"label"});
        left->pushBack(text);

        icon = new GuiIconElement();
        icon->setIcon(guiLoadIcon("svg/Entypo/archive.svg"));
        box = new GuiInputStringBox();
        box->setSize(gui::fill(), gui::em(1.70));
        box->setValue("./model.skeletal_model");

        right->pushBack(box);
        {
            auto buttons = right->pushBack(guiCreate<GuiElement>());
            buttons->setStyleClasses({ "container" });
            buttons->primary_axis = GUI_PRIMARY_AXIS::X;
            btn_browse = new GuiButton("", guiLoadIcon("svg/Entypo/folder.svg"));
            btn_reload = new GuiButton("", guiLoadIcon("svg/Entypo/ccw.svg"));
            btn_remove = new GuiButton("", guiLoadIcon("svg/Entypo/cross.svg"));

            buttons->pushBack(btn_browse);
            buttons->pushBack(btn_reload);
            buttons->pushBack(btn_remove);
        }
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