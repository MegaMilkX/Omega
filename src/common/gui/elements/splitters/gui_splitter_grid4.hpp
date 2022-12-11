#pragma once

#include "gui/elements/gui_element.hpp"

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
    GuiHitResult hitTest(int x, int y) override {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        GuiHitResult hit;
        if (elem_top_left) {
            hit = elem_top_left->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        if (elem_top_right) {
            hit = elem_top_right->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        if (elem_bottom_left) {
            hit = elem_bottom_left->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        if (elem_bottom_right) {
            hit = elem_bottom_right->hitTest(x, y);
            if (hit.hasHit()) {
                return hit;
            }
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }
    void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
    }
    void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags) override {
        bounding_rect = rc;
        client_area = bounding_rect;

        float offs_x = 200.0f;
        float offs_y = 30.0f;
        guiLayoutSplitRectX(rc, rc_top_left, rc_top_right, offs_x);
        guiLayoutSplitRectY(rc_top_left, rc_top_left, rc_bottom_left, offs_y);
        guiLayoutSplitRectY(rc_top_right, rc_top_right, rc_bottom_right, offs_y);


        if (elem_top_left) {
            elem_top_left->layout(rc_top_left.min, rc_top_left, flags);
        }
        if (elem_top_right) {
            elem_top_right->layout(rc_top_right.min, rc_top_right, flags);
        }
        if (elem_bottom_left) {
            elem_bottom_left->layout(rc_bottom_left.min, rc_bottom_left, flags);
        }
        if (elem_bottom_right) {
            elem_bottom_right->layout(rc_bottom_right.min, rc_bottom_right, flags);
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