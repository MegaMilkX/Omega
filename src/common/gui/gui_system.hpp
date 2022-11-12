#pragma once


#include "gui/elements/gui_element.hpp"
#include "gui/elements/gui_root.hpp"

#include "gui/gui_font.hpp"

void guiInit(Font* font);
void guiCleanup();

GuiRoot* guiGetRoot();

void guiPostMessage(GUI_MSG msg);
void guiPostMessage(GUI_MSG msg, uint64_t a, uint64_t b);
void guiPostMouseMove(int x, int y);
void guiPostResizingMessage(GuiElement* elem, GUI_HIT border, gfxm::rect rect);
void guiPostMovingMessage(GuiElement* elem, gfxm::rect rect);

void        guiSetActiveWindow(GuiElement* elem);
GuiElement* guiGetActiveWindow();
void        guiSetFocusedWindow(GuiElement* elem);
GuiElement* guiGetFocusedWindow();
GuiElement* guiGetHoveredElement(); 
// elem that was hovered when left mouse button wass pressed
// persists until left button is released or the element is destroyed
GuiElement* guiGetPressedElement(); 
// left mouse press followed by mouse move results in an element being "pulled"
// this status by itself does not affect the element in any way
GuiElement* guiGetPulledElement();

void guiBringWindowToTop(GuiElement* e);

void guiCaptureMouse(GuiElement* e);

void guiLayout();
void guiDraw();

bool guiIsDragDropInProgress();

int guiGetModifierKeysState();

bool guiClipboardGetString(std::string& out);
bool guiClipboardSetString(std::string str);

bool guiSetMousePos(int x, int y);
gfxm::vec2 guiGetMousePosLocal(const gfxm::rect& rc);

void guiPushFont(GuiFont* font);
void guiPopFont();
GuiFont* guiGetCurrentFont();
GuiFont* guiGetDefaultFont();

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