#pragma once

#include "gui/elements/text_element.hpp"
#include "gui/gui_system.hpp"
#include "IconsForkAwesome.h"


class GuiNotification : public GuiElement {
    GuiTextElement* icon = nullptr;
    GuiTextElement* label = nullptr;
public:
    GuiNotification(const std::string& text = "Notification") {
        setSize(gui::fill(), gui::em(4));
        setStyleClasses({ "notification-box" });

        primary_axis = GUI_PRIMARY_AXIS::X;

        icon = pushBack(guiCreate<GuiTextElement>(ICON_FK_EXCLAMATION_TRIANGLE));
        icon->setSize(gui::perc(25), gui::fill());
        icon->setStyleClasses({ "notification-icon" });
        
        label = pushBack(guiCreate<GuiTextElement>(text));
        label->setSize(gui::fill(), gui::fill());
        label->setStyleClasses({ "notification-text" });
    }
};

