#pragma once

#include "gui/elements/gui_element.hpp"


class GuiTimelineTrackBase : public GuiElement {
    
public:
    float content_offset_x = .0f;
    float frame_screen_width = 20.0f;

    int getFrameAtScreenPos(float x, float y) {
        return std::max(0.f, ((x + content_offset_x - 10.0f + (frame_screen_width * .5f)) / frame_screen_width) + .1f);
    }
    float getScreenXAtFrame(int frame) {
        return frame * frame_screen_width + client_area.min.x + 10.0f - content_offset_x;
    }

    GuiTimelineTrackBase() {}
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_SCROLL: {
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
        } break;
        case GUI_MSG::LBUTTON_DOWN:
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            break;
        case GUI_MSG::LBUTTON_UP:
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            break;
        case GUI_MSG::MBUTTON_DOWN:
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            break;
        case GUI_MSG::MBUTTON_UP:
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            break;
        case GUI_MSG::MOUSE_MOVE:
            if (getOwner()) {
                getOwner()->sendMessage(msg, params);
            }
            break;
        }
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;
    }
    void onDraw() override {}
};