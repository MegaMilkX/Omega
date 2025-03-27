#pragma once

#include <list>
#include "math/gfxm.hpp"


enum class GUI_HIT {
    ERR = -1,
    NOWHERE = 0,

    OUTSIDE_MENU, // Used to determine if a popup menu should close

    BORDER, // In the border of a window that does not have a sizing border.
    BOTTOM, // In the lower-horizontal border of a resizable window (the user can click the mouse to resize the window vertically).
    BOTTOMLEFT, // In the lower-left corner of a border of a resizable window (the user can click the mouse to resize the window diagonally).
    BOTTOMRIGHT, // In the lower-right corner of a border of a resizable window (the user can click the mouse to resize the window diagonally).
    CAPTION, // In a title bar.
    CLIENT, // In a client area.
    CLOSE, // In a Close button.
    GROWBOX, // In a size box
    HELP,  // In a Help button.
    HSCROLL, // In a horizontal scroll bar.
    LEFT, // In the left border of a resizable window (the user can click the mouse to resize the window horizontally).
    MENU, // In a menu.
    MAXBUTTON, // In a Maximize button.
    MINBUTTON, // In a Minimize button.
    REDUCE, // In a Minimize button.
    RIGHT, // In the right border of a resizable window(the user can click the mouse to resize the window horizontally).
    SIZE, // In a size box(same as GROWBOX).
    SYSMENU, // In a window menu or in a Close button in a child window.
    TOP, // In the upper - horizontal border of a window.
    TOPLEFT, //In the upper - left corner of a window border.
    TOPRIGHT, // In the upper - right corner of a window border.
    VSCROLL, // In the vertical scroll bar.
    ZOOM, // In a Maximize button.

    BOUNDING_RECT, // subject for removal

    DOCK_DRAG_DROP_TARGET,

    NATIVE_CAPTION
};

inline const char* guiHitTypeToString(GUI_HIT hit) {
    switch (hit) {
    case GUI_HIT::ERR: return "ERR";
    case GUI_HIT::NOWHERE: return "NOWHERE";

    case GUI_HIT::OUTSIDE_MENU: return "OUTSIDE_MENU";

    case GUI_HIT::BORDER: return "BORDER";
    case GUI_HIT::BOTTOM: return "BOTTOM";
    case GUI_HIT::BOTTOMLEFT: return "BOTTOMLEFT";
    case GUI_HIT::BOTTOMRIGHT: return "BOTTOMRIGHT";
    case GUI_HIT::CAPTION: return "CAPTION";
    case GUI_HIT::CLIENT: return "CLIENT";
    case GUI_HIT::CLOSE: return "CLOSE";
    case GUI_HIT::GROWBOX: return "GROWBOX";
    case GUI_HIT::HELP: return "HELP";
    case GUI_HIT::HSCROLL: return "HSCROLL";
    case GUI_HIT::LEFT: return "LEFT";
    case GUI_HIT::MENU: return "MENU";
    case GUI_HIT::MAXBUTTON: return "MAXBUTTON";
    case GUI_HIT::MINBUTTON: return "MINBUTTON";
    case GUI_HIT::REDUCE: return "REDUCE";
    case GUI_HIT::RIGHT: return "RIGHT";
    case GUI_HIT::SIZE: return "SIZE";
    case GUI_HIT::SYSMENU: return "SYSMENU";
    case GUI_HIT::TOP: return "TOP";
    case GUI_HIT::TOPLEFT: return "TOPLEFT";
    case GUI_HIT::TOPRIGHT: return "TOPRIGHT";
    case GUI_HIT::VSCROLL: return "VSCROLL";
    case GUI_HIT::ZOOM: return "ZOOM";

    case GUI_HIT::BOUNDING_RECT: return "BOUNDING_RECT";

    case GUI_HIT::DOCK_DRAG_DROP_TARGET: return "DOCK_DRAG_DROP_TARGET";

    case GUI_HIT::NATIVE_CAPTION: return "NATIVE_CAPTION";

    default: return "UNKNOWN";
    }
}


class GuiElement;

struct GuiHit {
    GUI_HIT hit;
    GuiElement* elem;
};

struct GuiHitResult {
    std::list<GuiHit> hits;
    bool hasHit() const { 
        if (hits.empty()) {
            return false;
        }
        return hits.back().hit != GUI_HIT::NOWHERE && hits.back().hit != GUI_HIT::OUTSIDE_MENU;
    }
    void add(GUI_HIT type, GuiElement* elem) {
        hits.push_back(GuiHit{ type, elem });
    }
    void clear() {
        hits.clear();
    }
};

void guiHitTestResizeBorders(GuiHitResult& hit, GuiElement* who, const gfxm::rect& rc, float border_thickness, int x, int y, char mask);

bool guiHitTestRect(const gfxm::rect& rc, const gfxm::vec2& pt);
bool guiHitTestCircle(const gfxm::vec2& pos, float radius, const gfxm::vec2& pt);
float guiHitTestLine3d(const gfxm::vec3& a, const gfxm::vec3& b, const gfxm::vec2& cursor, const gfxm::rect& viewport, const gfxm::mat4& transform, float& z);
