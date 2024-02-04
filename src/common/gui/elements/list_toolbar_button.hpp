#pragma once

#include "gui/elements/element.hpp"
#include "gui/gui_icon.hpp"
#include "gui/gui_system.hpp"


class GuiListToolbarButton : public GuiElement {
    GUI_MSG on_click_msg;
    GuiIcon* icon = 0;
public:
    GuiListToolbarButton(GuiIcon* icon, GUI_MSG on_click_msg) {
        setSize(gui::em(2), gui::em(2));
        this->icon = icon;
        this->on_click_msg = on_click_msg;
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(rc_bounds, gfxm::vec2(x, y))) {
            return;
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
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
