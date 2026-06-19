#pragma once

#include "gui/elements/element.hpp"
#include "gui/gui_icon.hpp"
#include "gui/gui_text_buffer.hpp"


class GuiButton : public GuiElement {
    const GuiIcon* icon = 0;
public:
    GuiButton(
        const char* caption = "Button",
        const GuiIcon* icon = 0
    ) {
        setMinSize(gui::em(2), gui::em(1.70));
        setSize(gui::content(), gui::em(1.70));
        setStyleClasses({ "control", "button" });
        primary_axis = GUI_PRIMARY_AXIS::X;

        pushBack(caption);

        this->icon = icon;
    }

    void setCaption(const char* cap) {
        clearChildren();
        pushBack(cap);
    }
    void setIcon(const GuiIcon* icon) {
        this->icon = icon;
    }
};

class GuiIconButton : public GuiElement {
    const GuiIcon* icon = 0;
public:
    GuiIconButton(const GuiIcon* icon) {
        setIcon(icon);
        setSize(gui::em(2), gui::em(2));
    }

    void setIcon(const GuiIcon* icon) {
        this->icon = icon;
    }

    void onDraw() override {
        Font* font = getFont();

        uint32_t col = GUI_COL_BUTTON;
        if (isPressed()) {
            col = GUI_COL_ACCENT;
        } else if (isHovered()) {
            col = GUI_COL_BUTTON_HOVER;
        }
        guiDrawRectRound(client_area, GUI_PADDING * 2.f, col);

        if (icon) {
            icon->draw(client_area, GUI_COL_WHITE);
        }
    }
};
