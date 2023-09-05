#pragma once

#include "gui/elements/element.hpp"


class GuiTimelineTrackBase : public GuiElement {
    bool is_locked = false;
public:
    float content_offset_x = .0f;
    float frame_screen_width = 20.0f;

    void setLocked(bool value) {
        is_locked = value;
    }
    bool isLocked() const {
        return is_locked;
    }

    int getFrameAtScreenPos(float x, float y) {
        return std::max(0.f, ((x + content_offset_x - 10.0f + (frame_screen_width * .5f)) / frame_screen_width) + .1f);
    }
    float getScreenXAtFrame(int frame) {
        return frame * frame_screen_width + client_area.min.x + 10.0f - content_offset_x;
    }

    GuiTimelineTrackBase() {}

    virtual const std::string& getName() const { return ""; }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        return false;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc_bounds;
    }
    void onDraw() override {}
};