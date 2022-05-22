#pragma once

#include <vector>
#include "common/math/gfxm.hpp"
#include "common/gui/gui_font.hpp"
#include "common/gui/gui_draw.hpp"
#include "common/gui/gui_values.hpp"
#include "common/gui/gui_color.hpp"
#include "platform/platform.hpp"

enum class GUI_MSG {
    PAINT,
    MOUSE_MOVE,
    MOUSE_ENTER,
    MOUSE_LEAVE,
    LBUTTON_DOWN,
    LBUTTON_UP,

    KEYDOWN,
    KEYUP,

    UNICHAR,

    ACTIVATE,
    DEACTIVATE,

    FOCUS,
    UNFOCUS,

    CLICKED,    // left mouse buttton pressed and released while hovering the same element
    DBL_CLICKED,
    PULL_START, // user has pressed down the left mouse button and moved the mouse
    PULL,       // for any mouse move while pulling is in action
    PULL_STOP,  // 

    // SCROLL BAR
    SB_THUMB_TRACK,

    MOVING,
    RESIZING,

    NOTIFY,

    DOCK_TAB_DRAG_START,
    DOCK_TAB_DRAG_STOP,
    DOCK_TAB_DRAG_ENTER,
    DOCK_TAB_DRAG_LEAVE,
    DOCK_TAB_DRAG_HOVER,
    DOCK_TAB_DRAG_DROP_PAYLOAD,
    DOCK_TAB_DRAG_DROP_PAYLOAD_SPLIT,
    DOCK_TAB_DRAG_SUCCESS,  // Received by the sender when drag-drop operation finished successfully
    DOCK_TAB_DRAG_FAIL      // Received by the sender when drag-drop operation failed or was cancelled
};

enum class GUI_NOTIFICATION {
    NONE,
    TAB_CLICKED,
    DRAG_TAB_START,
    DRAG_TAB_END,
    DRAG_DROP_TARGET_HOVERED
};

enum class GUI_DOCK {
    NONE = 0b0000,
    LEFT = 0b0001,
    RIGHT = 0b0010,
    TOP = 0b0100,
    BOTTOM = 0b1000,
    FILL = 0b1111
};

enum class GUI_HIT {
    ERR = -1,
    NOWHERE = 0,

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

    DOCK_DRAG_DROP_TARGET
};

enum class GUI_DOCK_SPLIT_DROP {
    NONE,
    MID,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM
};

enum class GUI_CHAR {
    BACKSPACE = 0x08,
    LINEFEED  = 0x0A, // shift+enter
    ESCAPE    = 0x1B,
    TAB       = 0x09,
    RETURN    = 0x0D, // return
};

const int GUI_KEY_CONTROL = 0b0001;
const int GUI_KEY_ALT = 0b0010;
const int GUI_KEY_SHIFT = 0b0100;

const uint64_t GUI_LAYOUT_NO_TITLE  = 0x00000001;
const uint64_t GUI_LAYOUT_NO_BORDER = 0x00000002;

const uint64_t GUI_FLAG_OVERLAPPED  = 0x00000001;
const uint64_t GUI_FLAG_TOPMOST     = 0x00000002;

class GuiElement;

struct GuiHitResult {
    GUI_HIT hit;
    GuiElement* elem;
};

class GuiElement {
    int z_order = 0;
    bool is_enabled = true;
    uint64_t flags = 0x0;
    GuiFont* font = 0;
protected:
    std::vector<GuiElement*> children;
    GuiElement* parent = 0;
    GuiElement* owner = 0;

    gfxm::rect bounding_rect = gfxm::rect(0, 0, 0, 0);
    gfxm::rect client_area = gfxm::rect(0, 0, 0, 0);

public:
    int         getZOrder() const { return z_order; }
    bool        isEnabled() const { return is_enabled; }
    void        setEnabled(bool enabled) { is_enabled = enabled; }
    uint64_t    getFlags() const { return flags; }
    void        setFlags(uint64_t f) { flags = f; }
    GuiFont*    getFont() { return font; }

    GuiElement*         getParent() { return parent; }
    const GuiElement*   getParent() const { return parent; }
    const gfxm::rect&   getBoundingRect() const { return bounding_rect; }
    const gfxm::rect&   getClientArea() const { return client_area; }
    
    GuiElement* getOwner() { return owner; }
    void        setOwner(GuiElement* elem) { owner = elem; }

    void bringToTop(GuiElement* e) {
        assert(e->parent == this);
        if (e->parent != this) {
            return;
        }
        int highest_z = -1;
        for (int i = 0; i < children.size(); ++i) {
            if (highest_z < children[i]->z_order) {
                highest_z = children[i]->z_order;
            }
        }
        e->z_order = highest_z + 1;
    }
public:
    gfxm::vec2 pos = gfxm::vec2(100.0f, 100.0f);
    gfxm::vec2 size = gfxm::vec2(150.0f, 100.0f);

    GuiElement();
    virtual ~GuiElement();

    bool isHovered() const;
    bool isPressed() const;
    bool isPulled() const;

    void layout(const gfxm::rect& rc, uint64_t flags);
    void draw();

    void sendMessage(GUI_MSG msg, uint64_t a, uint64_t b);

    virtual GuiHitResult hitTest(int x, int y) {
        if (gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::CLIENT, this };
        }
        else if (gfxm::point_in_rect(bounding_rect, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::BOUNDING_RECT, this };
        }
        else {
            return GuiHitResult{ GUI_HIT::NOWHERE, this };
        }
    }

    virtual void onMessage(GUI_MSG msg, uint64_t a_param, uint64_t b_param) {
        switch (msg) {
        case GUI_MSG::PAINT: {
        } break;
        }
    }

    virtual void onLayout(const gfxm::rect& rect, uint64_t flags) {
        this->bounding_rect = rect;
        this->client_area = bounding_rect;

        gfxm::rect rc = rect;
        for (auto& ch : children) {
            GUI_DOCK dock_pos = ch->getDockPosition();
            gfxm::rect new_rc = rc;
            if (dock_pos == GUI_DOCK::NONE) {
                new_rc = gfxm::rect(
                    ch->pos,
                    ch->pos + ch->size
                );
            }
            else if (dock_pos == GUI_DOCK::LEFT) {
                new_rc.max.x = rc.min.x + ch->size.x;
                rc.min.x = new_rc.max.x;
            }
            else if (dock_pos == GUI_DOCK::RIGHT) {
                new_rc.min.x = rc.max.x - ch->size.x;
                rc.max.x = new_rc.min.x;
            }
            else if (dock_pos == GUI_DOCK::TOP) {
                new_rc.max.y = rc.min.y + ch->size.y;
                rc.min.y = new_rc.max.y;
            }
            else if (dock_pos == GUI_DOCK::BOTTOM) {
                new_rc.min.y = rc.max.y - ch->size.y;
                rc.max.y = new_rc.min.y;
            }
            else if (dock_pos == GUI_DOCK::FILL) {

            }
            ch->layout(new_rc, 0);
        }
    }

    virtual void onDraw() {
        guiDrawPushScissorRect(client_area);
        for (auto& ch : children) {
            ch->draw();
        }

        //guiDrawRectLine(bounding_rect);
        guiDrawPopScissorRect();
    }

    virtual void addChild(GuiElement* elem);
    virtual void removeChild(GuiElement* elem);
    size_t childCount() const;
    GuiElement* getChild(int i);
    int getChildId(GuiElement* elem);

    virtual GuiElement* getScrollBarV() { return 0; }
    virtual GuiElement* getScrollBarH() { return 0; }

    virtual GUI_DOCK getDockPosition() const {
        return GUI_DOCK::NONE;
    }
    virtual void setDockPosition(GUI_DOCK dock) {}

    virtual bool isDockable() const {
        return false;
    }
    virtual void setDockable(bool is_dockable) {}
};