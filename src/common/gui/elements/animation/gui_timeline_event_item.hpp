#pragma once

#include "gui/elements/element.hpp"


class GuiTimelineEventItem : public GuiElement {
    float radius = 7.f;
    bool is_dragging = false;
public:
    int frame = 0;
    void* user_ptr = 0;

    GuiTimelineEventItem(int at)
        : frame(at) {}
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        // TODO: FIX UNITS
        if (!guiHitTestCircle(gfxm::vec2(pos.x.value, pos.y.value), radius, gfxm::vec2(x, y))) {
            return;
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
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
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        gfxm::rect rc_(
            -gfxm::vec2(radius, radius),
            gfxm::vec2(radius, radius)
        );
        // TODO: FIX UNITS
        pos.x.value = 0;
        pos.y.value = 0;
        rc_bounds = rc_;
        client_area = rc_;
    }
    void onDraw() override {
        uint32_t color = GUI_COL_TIMELINE_CURSOR;
        if (isHovered() || is_dragging) {
            color = GUI_COL_TEXT;
        }
        guiDrawDiamond(gfxm::vec2(pos.x.value, pos.y.value), radius, color, color, color);
    }
};

