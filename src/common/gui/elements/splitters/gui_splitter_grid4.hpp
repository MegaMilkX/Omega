#pragma once

#include "gui/elements/element.hpp"

class GuiSplitterGrid4 : public GuiElement {
    GuiElement* elem_top_left = 0;
    GuiElement* elem_top_right = 0;
    GuiElement* elem_bottom_left = 0;
    GuiElement* elem_bottom_right = 0;
    gfxm::rect rc_top_left;
    gfxm::rect rc_top_right;
    gfxm::rect rc_bottom_left;
    gfxm::rect rc_bottom_right;
public:
    void setElemTopLeft(GuiElement* elem) {
        if (elem_top_left) {
            removeChild(elem_top_left);
        }
        elem_top_left = elem;
        addChild(elem);
    }
    void setElemTopRight(GuiElement* elem) {
        if (elem_top_right) {
            removeChild(elem_top_right);
        }
        elem_top_right = elem;
        addChild(elem);
    }
    void setElemBottomLeft(GuiElement* elem) {
        if (elem_bottom_left) {
            removeChild(elem_bottom_left);
        }
        elem_bottom_left = elem;
        addChild(elem);
    }
    void setElemBottomRight(GuiElement* elem) {
        if (elem_bottom_right) {
            removeChild(elem_bottom_right);
        }
        elem_bottom_right = elem;
        addChild(elem);
    }
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return;
        }
        if (elem_top_left) {
            elem_top_left->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        if (elem_top_right) {
            elem_top_right->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        if (elem_bottom_left) {
            elem_bottom_left->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        if (elem_bottom_right) {
            elem_bottom_right->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override { return false; }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        rc_bounds = rc;
        client_area = rc_bounds;

        float offs_x = 200.0f;
        float offs_y = 30.0f;
        guiLayoutSplitRectX(rc, rc_top_left, rc_top_right, offs_x);
        guiLayoutSplitRectY(rc_top_left, rc_top_left, rc_bottom_left, offs_y);
        guiLayoutSplitRectY(rc_top_right, rc_top_right, rc_bottom_right, offs_y);


        if (elem_top_left) {
            elem_top_left->layout(rc_top_left, flags);
        }
        if (elem_top_right) {
            elem_top_right->layout(rc_top_right, flags);
        }
        if (elem_bottom_left) {
            elem_bottom_left->layout(rc_bottom_left, flags);
        }
        if (elem_bottom_right) {
            elem_bottom_right->layout(rc_bottom_right, flags);
        }
    }
    void onDraw() override {
        if (elem_top_left) {
            elem_top_left->draw();
        }
        if (elem_top_right) {
            elem_top_right->draw();
        }
        if (elem_bottom_left) {
            elem_bottom_left->draw();
        }
        if (elem_bottom_right) {
            elem_bottom_right->draw();
        }
    }
};