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
    GuiHitResult hitTest(int x, int y) override {
        if (!guiHitTestCircle(pos, radius, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::CLICKED:
        case GUI_MSG::DBL_CLICKED:
            notifyOwner<int, GuiTimelineEventItem*>(GUI_NOTIFY::TIMELINE_EVENT_SELECTED, 0, this);
            break;
        case GUI_MSG::MOUSE_MOVE:
            if (is_dragging) {
                notifyOwner(GUI_NOTIFY::TIMELINE_DRAG_EVENT, this);
            }
            break;
        case GUI_MSG::LBUTTON_DOWN:
            guiCaptureMouse(this);
            is_dragging = true;
            break;
        case GUI_MSG::LBUTTON_UP:
            guiCaptureMouse(0);
            is_dragging = false;
            break;
        case GUI_MSG::RBUTTON_DOWN:
            notifyOwner(GUI_NOTIFY::TIMELINE_ERASE_EVENT, this);
            break;
        }

    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        gfxm::rect rc_(
            cursor - gfxm::vec2(radius, radius),
            cursor + gfxm::vec2(radius, radius)
        );
        pos = cursor;
        bounding_rect = rc_;
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
