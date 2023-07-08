#pragma once

#include "gui/elements/gui_element.hpp"


class GuiWindowTitleBarButton : public GuiElement {
    GUI_MSG on_click_msg;
    GuiIcon* icon = 0;
public:
    GuiWindowTitleBarButton(GuiIcon* icon, GUI_MSG on_click_msg) {
        this->icon = icon;
        this->on_click_msg = on_click_msg;
    }
    GuiHitResult onHitTest(int x, int y) override {
        if (!gfxm::point_in_rect(rc_bounds, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK: {
            guiPostMessage(getOwner(), on_click_msg);
            return true;
        }
        }
        return false;
    }
    void onLayout(const gfxm::rect& rect, uint64_t flags) override {
        rc_bounds = rect;
        client_area = rc_bounds;
    }
    void onDraw() override {
        uint32_t color = GUI_COL_BLACK;
        if (isHovered()) {
            color = GUI_COL_TEXT;
        }
        if (icon) {
            icon->draw(client_area, color);
        }
    }
};
