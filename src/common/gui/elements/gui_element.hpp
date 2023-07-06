#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "gui/gui_font.hpp"
#include "gui/gui_draw.hpp"
#include "gui/gui_values.hpp"
#include "gui/gui_color.hpp"
#include "platform/platform.hpp"

#include "gui/gui_msg.hpp"

// forward declaration
void guiSendMessage(GuiElement* target, GUI_MSG msg, GUI_MSG_PARAMS params);

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

typedef uint64_t gui_flag_t;
const gui_flag_t GUI_FLAG_OVERLAPPED                = 0x00000001;
const gui_flag_t GUI_FLAG_TOPMOST                   = 0x00000002;
const gui_flag_t GUI_FLAG_BLOCKING                  = 0x00000004;
const gui_flag_t GUI_FLAG_SAME_LINE                 = 0x00000008;
const gui_flag_t GUI_FLAG_SCROLLV                   = 0x00000010;
const gui_flag_t GUI_FLAG_SCROLLH                   = 0x00000020;


enum class GUI_LAYOUT {
    STACK_LEFT,
    STACK_TOP,
    FLOAT,
    FILL_H,
    FILL_V,
    FILL
};

class GuiElement;

struct GuiHitResult {
    GUI_HIT hit;
    GuiElement* elem;
    bool hasHit() const { return hit != GUI_HIT::NOWHERE; }
};

class GuiElement {
    friend void guiLayout();
    friend void guiDraw();

    int z_order = 0;
    bool is_enabled = true;
    gui_flag_t flags = 0x0;
    GuiFont* font = 0;
protected:
    std::vector<GuiElement*> children;
    GuiElement* parent = 0;
    GuiElement* owner = 0;

    gfxm::rect rc_bounds = gfxm::rect(0, 0, 0, 0);
    gfxm::rect client_area = gfxm::rect(0, 0, 0, 0);
    // Content rectangle local to the container
    gfxm::rect rc_content = gfxm::rect(.0f, .0f, .0f, .0f);
    // Content offset local to the container
    gfxm::vec2 pos_content;

    gfxm::mat4  content_view_transform_world = gfxm::mat4(1.f);
    gfxm::vec3  content_view_translation = gfxm::vec3(0, 0, 0);
    float       content_view_scale = 1.f;

    void forwardMessageToOwner(GUI_MSG msg, GUI_MSG_PARAMS params) {
        assert(getOwner());
        if (getOwner()) {
            getOwner()->sendMessage(msg, params);
        }
    }

    int getContentHeight() const {
        return rc_content.max.y - rc_content.min.y;
    }
    int getContentWidth() const {
        return rc_content.max.x - rc_content.min.x;
    }
    int getClientHeight() const {
        return client_area.max.y - client_area.min.y;
    }
    int getClientWidth() const {
        return client_area.max.x - client_area.min.x;
    }

    void layoutContentTopDown(const gfxm::rect& rc) {
        gfxm::rect rc_ = rc;
        rc_.min += content_padding.min;
        rc_.max -= content_padding.max;
        rc_content = rc_;

        gfxm::vec2 pt_current_line = gfxm::vec2(rc_.min.x, rc_.min.y) - pos_content;
        gfxm::vec2 pt_next_line = pt_current_line;
        //rc_content = gfxm::rect(.0f, .0f, .0f, .0f);
        for (auto& ch : children) {
            gfxm::vec2 pos;
            if ((ch->getFlags() & GUI_FLAG_SAME_LINE) && rc_.max.x - pt_current_line.x >= ch->min_size.x) {
                pos = pt_current_line;
            } else {
                pos = pt_next_line;
                pt_current_line = pt_next_line;
            }
            float width = ch->size.x;
            float height = ch->size.y;
            if (width == .0f) {
                width = gfxm::_max(.0f, rc_.max.x - pos_content.x - pos.x);
            }
            if (height == .0f) {
                height = gfxm::_max(.0f, rc_.max.y - pos_content.y - pos.y);
            }
            gfxm::rect rect(pos, pos + gfxm::vec2(width, height));
            //y += ch->size.y + GUI_PADDING;
            //rc_content.max.y += ch->size.y + GUI_PADDING;

            ch->layout(rect, 0);
            pt_next_line = gfxm::vec2(rc_.min.x - pos_content.x, gfxm::_max(pt_next_line.y - pos_content.y, ch->getBoundingRect().max.y + GUI_PADDING));
            pt_current_line.x = ch->getBoundingRect().max.x + GUI_PADDING;
            //y = ch->getBoundingRect().max.y + GUI_PADDING;
            //rc_content.max.y = ch->getBoundingRect().max.y + GUI_PADDING;
            rc_content.max.y = gfxm::_max(rc_content.max.y, ch->getBoundingRect().max.y);
            rc_content.max.x = gfxm::_max(rc_content.max.x, ch->getBoundingRect().max.x);
        }
        rc_content.min = rc_.min - pos_content;
        if ((getFlags() & GUI_FLAG_SCROLLV) == 0 && getContentHeight() > getClientHeight()) {
            addFlags(GUI_FLAG_SCROLLV);
        } else if((getFlags() & GUI_FLAG_SCROLLV) && getContentHeight() <= getClientHeight()) {
            removeFlags(GUI_FLAG_SCROLLV);
        }
        if ((getFlags() & GUI_FLAG_SCROLLH) == 0 && getContentWidth() > getClientWidth()) {
            addFlags(GUI_FLAG_SCROLLH);
        } else if((getFlags() & GUI_FLAG_SCROLLH) && getContentWidth() <= getClientWidth()) {
            removeFlags(GUI_FLAG_SCROLLH);
        }

        if (pos_content.y > .0f && rc_content.max.y < client_area.max.y) {
            pos_content.y = gfxm::_max(.0f, pos_content.y - (client_area.max.y - rc_content.max.y));
        }
        if (pos_content.x > .0f && rc_content.max.x < client_area.max.x) {
            pos_content.x = gfxm::_max(.0f, pos_content.x - (client_area.max.x - rc_content.max.x));
        }
    }
    void drawContent() {
        guiDrawPushScissorRect(client_area);
        for (int i = 0; i < children.size(); ++i) {
            auto c = children[i];
            c->draw();
            //guiDrawRectLine(c->getBoundingRect(), GUI_COL_BLUE);
        }
        guiDrawPopScissorRect();
    }
public:
    uint64_t sys_flags = 0x0;

    bool is_hidden = false;
    gfxm::vec2 pos = gfxm::vec2(100.0f, 100.0f);
    gfxm::vec2 size = gfxm::vec2(350.0f, 100.0f);
    gfxm::vec2 min_size = gfxm::vec2(.0f, .0f);
    gfxm::vec2 max_size = gfxm::vec2(FLT_MAX, FLT_MAX);
    gfxm::rect content_padding = gfxm::rect(GUI_PADDING, GUI_PADDING, GUI_PADDING, GUI_PADDING);
    gfxm::rect margin = gfxm::rect(GUI_MARGIN, GUI_MARGIN, GUI_MARGIN, GUI_MARGIN);
    
    void setWidth(float w) { size.x = w; }
    void setHeight(float h) { size.y = h; }
    void setSize(float w, float h) { size = gfxm::vec2(w, h); }
    void setSize(gfxm::vec2 sz) { size = sz; }
    void setPosition(float x, float y) { pos = gfxm::vec2(x, y); }
    void setPosition(gfxm::vec2 p) { pos = p; }

    const gfxm::rect& getLocalContentRect() const { return rc_content; }
    const gfxm::vec2& getLocalContentOffset() const { return pos_content; }
    void setLocalContentOffset(const gfxm::vec2& pos) { pos_content = pos; }

    int         getZOrder() const { return z_order; }
    bool        isEnabled() const { return is_enabled; }
    void        setEnabled(bool enabled) { is_enabled = enabled; }
    gui_flag_t  getFlags() const { return flags; }
    void        setFlags(gui_flag_t f) { flags = f; }
    void        addFlags(gui_flag_t f) { flags = flags | f; }
    void        removeFlags(gui_flag_t f) { flags = flags & ~f; }
    GuiFont*    getFont() { return font; }

    GuiElement*         getParent() { return parent; }
    const GuiElement*   getParent() const { return parent; }
    const gfxm::rect&   getBoundingRect() const { return rc_bounds; }
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
    
    GuiElement*     getOwner() { return owner; }
    void            setOwner(GuiElement* elem) { owner = elem; }
    virtual void    setParent(GuiElement* elem) { parent = elem; }

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

    virtual void hitTest(int x, int y);
    virtual void layout(const gfxm::rect& rc, uint64_t flags);
    virtual void draw();

    GuiElement* sendMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        GuiElement* elem = this;
        while (elem) {
            if (elem->onMessage(msg, params)) {
                return elem;
            }
            if (msg == GUI_MSG::NOTIFY) {
                elem = elem->getOwner();
            } else {
                elem = elem->getParent();
            }
        }
        guiSendMessage(0, msg, params);
        return 0;
    }
    template<typename TYPE_A, typename TYPE_B>
    GuiElement* sendMessage(GUI_MSG msg, const TYPE_A& a, const TYPE_B& b) {
        GUI_MSG_PARAMS params;
        params.setA(a);
        params.setB(b);
        return sendMessage(msg, params);
    }
    template<typename TYPE_A, typename TYPE_B, typename TYPE_C>
    GuiElement* sendMessage(GUI_MSG msg, const TYPE_A& a, const TYPE_B& b, const TYPE_C& c) {
        GUI_MSG_PARAMS params;
        params.setA(a);
        params.setB(b);
        params.setC(c);
        return sendMessage(msg, params);
    }
    template<typename T>
    GuiElement* notify(GUI_NOTIFY t, T b_param) {
        return sendMessage<GUI_NOTIFY, T>(GUI_MSG::NOTIFY, t, b_param);
    }
    template<typename T, typename T2>
    GuiElement* notify(GUI_NOTIFY t, T b_param, T2 c_param) {
        return sendMessage<GUI_NOTIFY, T, T2>(GUI_MSG::NOTIFY, t, b_param, c_param);
    }
    template<typename T>
    bool notifyOwner(GUI_NOTIFY t, const T& b_param) {
        if (getOwner()) {
            getOwner()->notify(t, b_param);
            return true;
        } else {
            return false;
        }
    }
    template<typename T, typename T2>
    bool notifyOwner(GUI_NOTIFY t, const T& b_param, const T2& c_param) {
        if (getOwner()) {
            getOwner()->notify(t, b_param, c_param);
            return true;
        } else {
            return false;
        }
    }

    virtual bool isContainer() const { return false; }

    virtual GuiHitResult onHitTest(int x, int y) {
        if (!gfxm::point_in_rect(rc_bounds, gfxm::vec2(x, y))) {
            return GuiHitResult{ GUI_HIT::NOWHERE, 0 };
        }
        
        if (gfxm::point_in_rect(client_area, gfxm::vec2(x, y))) {
            for (auto& ch : children) {
                GuiHitResult hit = ch->onHitTest(x, y);
                if (hit.hasHit()) {
                    return hit;
                }
            }
        }

        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }

    virtual bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        return false;
    }

    virtual void onLayout(const gfxm::rect& rect, uint64_t flags) {
        rc_bounds = rect;
        client_area = rc_bounds;
        gfxm::expand(client_area, content_padding);


        float y = client_area.min.y;
        for (auto& ch : children) {
            float width = client_area.max.x - client_area.min.x;
            float height = ch->size.y;
            if (height == .0f) {
                height = client_area.max.y - client_area.min.y;
            }
            gfxm::vec2 pos = gfxm::vec2(client_area.min.x, y);
            ch->size.x = client_area.max.x - client_area.min.x;
            gfxm::rect rect(pos, pos + gfxm::vec2(width, height));
            y += ch->size.y + GUI_PADDING;

            ch->layout(rect, 0);
        }
    }

    virtual void onDraw() {
        for (auto& ch : children) {
            ch->draw();
        }
    }

    virtual void clearChildren() {
        for (int i = 0; i < children.size(); ++i) {
            children[i]->parent = 0;
        }
        children.clear();
    }
    virtual void addChild(GuiElement* elem) {
        assert(elem != this);
        if (elem->getParent()) {
            elem->getParent()->removeChild(elem);
        }
        int new_z_order = children.size();
        children.push_back(elem);
        elem->parent = this;
        elem->z_order = new_z_order;
        if (elem->owner == 0) {
            elem->setOwner(this);
        }
    }
    virtual void removeChild(GuiElement* elem) {
        int id = -1;
        for (int i = 0; i < children.size(); ++i) {
            if (children[i] == elem) {
                id = i;
                break;
            }
        }
        if (id >= 0) {
            children[id]->parent = 0;
            children.erase(children.begin() + id);
        }
    }
    size_t GuiElement::childCount() const {
        return children.size();
    }
    GuiElement* GuiElement::getChild(int i) {
        return children[i];
    }
    int GuiElement::getChildId(GuiElement* elem) {
        int id = -1;
        for (int i = 0; i < children.size(); ++i) {
            if (children[i] == elem) {
                id = i;
                break;
            }
        }
        return id;
    }

    virtual GUI_DOCK getDockPosition() const {
        return GUI_DOCK::NONE;
    }
    virtual void setDockPosition(GUI_DOCK dock) {}

    virtual bool isDockable() const {
        return false;
    }
    virtual void setDockable(bool is_dockable) {}
};