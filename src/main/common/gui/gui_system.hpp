#pragma once


#include "common/gui/elements/gui_element.hpp"

void guiPostMessage(GUI_MSG msg);
void guiPostMouseMove(int x, int y);
void guiPostResizingMessage(GuiElement* elem, GUI_HIT border, gfxm::rect rect);

void guiCaptureMouse(GuiElement* e);

void guiLayout();
void guiDraw(Font* font);

inline void guiCalcResizeBorders(const gfxm::rect& rect, float thickness_outer, float thickness_inner, gfxm::rect* left, gfxm::rect* right, gfxm::rect* top, gfxm::rect* bottom) {
    assert(left && right && top && bottom);

    left->min = gfxm::vec2(
        rect.min.x - thickness_outer,
        rect.min.y - thickness_outer
    );
    left->max = gfxm::vec2(
        rect.min.x + thickness_inner,
        rect.max.y + thickness_outer
    );

    right->min = gfxm::vec2(
        rect.max.x - thickness_inner,
        rect.min.y - thickness_outer
    );
    right->max = gfxm::vec2(
        rect.max.x + thickness_outer,
        rect.max.y + thickness_outer
    );

    top->min = gfxm::vec2(
        rect.min.x - thickness_outer,
        rect.min.y - thickness_outer
    );
    top->max = gfxm::vec2(
        rect.max.x + thickness_outer,
        rect.min.y + thickness_inner
    );

    bottom->min = gfxm::vec2(
        rect.min.x - thickness_outer,
        rect.max.y - thickness_inner
    );
    bottom->max = gfxm::vec2(
        rect.max.x + thickness_outer,
        rect.max.y + thickness_outer
    );
}