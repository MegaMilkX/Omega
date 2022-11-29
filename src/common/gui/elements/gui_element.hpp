#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "gui/gui_font.hpp"
#include "gui/gui_draw.hpp"
#include "gui/gui_values.hpp"
#include "gui/gui_color.hpp"
#include "platform/platform.hpp"

#include "gui/gui_msg.hpp"

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
const uint64_t GUI_LAYOUT_DRAW_SHADOW = 0x00000004;

const uint64_t GUI_FLAG_OVERLAPPED  = 0x00000001;
const uint64_t GUI_FLAG_TOPMOST     = 0x00000002;

class GuiElement;

struct GuiHitResult {
    GUI_HIT hit;
    GuiElement* elem;
    bool hasHit() const { return hit != GUI_HIT::NOWHERE; }
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

    gfxm::mat4  content_view_transform_world = gfxm::mat4(1.f);
    gfxm::vec3  content_view_translation = gfxm::vec3(0, 0, 0);
    float       content_view_scale = 1.f;

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

    void setContentViewTranslation(const gfxm::vec3& t) { content_view_translation = t; }
    void translateContentView(float x, float y) { content_view_translation += gfxm::vec3(x, y, .0f); }
    void setContentViewScale(float s) { content_view_scale = s; }
    const gfxm::vec3& getContentViewTranslation() const { return content_view_translation; }
    float getContentViewScale() const { return content_view_scale; }
    gfxm::mat4 getContentViewTransform() {
        content_view_transform_world
            = gfxm::translate(gfxm::mat4(1.f), content_view_translation)
            * gfxm::scale(gfxm::mat4(1.f), gfxm::vec3(content_view_scale, content_view_scale, 1.f));
        if (parent) {
            content_view_transform_world = parent->getContentViewTransform() * content_view_transform_world;
        }
        return gfxm::inverse(content_view_transform_world);
    }
    
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
    gfxm::vec2 min_size = gfxm::vec2(.0f, .0f);
    gfxm::vec2 max_size = gfxm::vec2(FLT_MAX, FLT_MAX);
    gfxm::rect content_padding = gfxm::rect(GUI_PADDING, GUI_PADDING, GUI_PADDING, GUI_PADDING);

    GuiElement();
    virtual ~GuiElement();

    void setSize(int w, int h) { size = gfxm::vec2(w, h); }
    void setPosition(int x, int y) { pos = gfxm::vec2(x, y); }
    void setMinSize(int w, int h) { min_size = gfxm::vec2(w, h); }
    void setMaxSize(int w, int h) { max_size = gfxm::vec2(w, h); }

    bool isHovered() const;
    bool isPressed() const;
    bool isPulled() const;
    bool hasMouseCapture() const;

    void layout(const gfxm::vec2& cursor, const gfxm::rect& rc, uint64_t flags);
    void draw();

    void sendMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        onMessage(msg, params);
    }
    template<typename TYPE_A, typename TYPE_B>
    void sendMessage(GUI_MSG msg, const TYPE_A& a, const TYPE_B& b) {
        GUI_MSG_PARAMS params;
        params.setA(a);
        params.setB(b);
        sendMessage(msg, params);
    }
    template<typename TYPE_A, typename TYPE_B, typename TYPE_C>
    void sendMessage(GUI_MSG msg, const TYPE_A& a, const TYPE_B& b, const TYPE_C& c) {
        GUI_MSG_PARAMS params;
        params.setA(a);
        params.setB(b);
        params.setC(c);
        sendMessage(msg, params);
    }
    template<typename T>
    void notify(GUI_NOTIFICATION t, T b_param) {
        sendMessage<GUI_NOTIFICATION, T>(GUI_MSG::NOTIFY, t, b_param);
    }
    template<typename T, typename T2>
    void notify(GUI_NOTIFICATION t, T b_param, T2 c_param) {
        sendMessage<GUI_NOTIFICATION, T, T2>(GUI_MSG::NOTIFY, t, b_param, c_param);
    }
    template<typename T>
    bool notifyOwner(GUI_NOTIFICATION t, const T& b_param) {
        if (getOwner()) {
            getOwner()->notify(t, b_param);
            return true;
        } else {
            return false;
        }
    }
    template<typename T, typename T2>
    bool notifyOwner(GUI_NOTIFICATION t, const T& b_param, const T2& c_param) {
        if (getOwner()) {
            getOwner()->notify(t, b_param, c_param);
            return true;
        } else {
            return false;
        }
    }

    virtual GuiHitResult hitTest(int x, int y) {
        if (!gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        return GuiHitResult{ GUI_HIT::CLIENT, this };/*
        if (gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::CLIENT, this };
        }
        else if (gfxm::point_in_rect(bounding_rect, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::BOUNDING_RECT, this };
        }
        else {
            return GuiHitResult{ GUI_HIT::NOWHERE, this };
        }*/
    }

    virtual void onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::PAINT: {
        } break;
        }
    }

    virtual void onLayout(const gfxm::vec2& cursor, const gfxm::rect& rect, uint64_t flags) {
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
            ch->layout(cursor, new_rc, 0);
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