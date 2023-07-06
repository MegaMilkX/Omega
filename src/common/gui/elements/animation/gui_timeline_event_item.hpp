#pragma once

#include "gui/elements/gui_element.hpp"


class GuiTimelineEventItem : public GuiElement {
    float radius = 7.f;
    bool is_dragging = false;
public:
    int frame = 0;
    void* user_ptr = 0;

    GuiTimelineEventItem(int at)
        : frame(at) {}
    GuiHitResult onHitTest(int x, int y) override {
        if (!guiHitTestCircle(pos, radius, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK:
            notifyOwner<int, GuiTimelineEventItem*>(GUI_NOTIFY::TIMELINE_EVENT_SELECTED, 0, this);
            return true;
        case GUI_MSG::MOUSE_MOVE:
            if (is_dragging) {
                notifyOwner(GUI_NOTIFY::TIMELINE_DRAG_EVENT, this);
            }
            return true;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            is_dragging = true;
            return true;
        case GUI_MSG::LBUTTON_UP:
            guiCaptureMouse(0);
            is_dragging = false;
            return true;
        case GUI_MSG::RBUTTON_DOWN:
            notifyOwner(GUI_NOTIFY::TIMELINE_ERASE_EVENT, this);
            return true;
        }
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        gfxm::rect rc_(
            rc.min - gfxm::vec2(radius, radius),
            rc.min + gfxm::vec2(radius, radius)
        );
        pos = rc.min;
        rc_bounds = rc_;
        client_area = rc_;
    }
    void onDraw() override {
        uint32_t color = GUI_COL_TEXT;
        if (isHovered() || is_dragging) {
            color = GUI_COL_TIMELINE_CURSOR;
        }
        guiDrawDiamond(pos, radius, color, color, color);
    }
};

