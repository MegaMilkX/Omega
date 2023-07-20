#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "gui/gui_font.hpp"
#include "gui/gui_draw.hpp"
#include "gui/gui_values.hpp"
#include "gui/gui_color.hpp"
#include "gui/types.hpp"
#include "platform/platform.hpp"

#include "gui/gui_msg.hpp"

// forward declaration
void guiSendMessage(GuiElement* target, GUI_MSG msg, GUI_MSG_PARAMS params);
void guiCaptureMouse(GuiElement* e);
bool guiShowContextPopup(GuiElement* owner, int x, int y);

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

const uint64_t GUI_SYS_FLAG_DRAG_SUBSCRIBER = 0x0001;
const uint64_t GUI_SYS_FLAG_HAS_CONTEXT_POPUP = 0x0002;

typedef uint64_t gui_flag_t;
// Stops an element from being deleted by clearChildren()
const gui_flag_t GUI_FLAG_PERSISTENT                = 0x00000001;
// Frame layout
const gui_flag_t GUI_FLAG_FRAME                     = 0x00000002;
// Floating layout
const gui_flag_t GUI_FLAG_FLOATING                  = 0x00000004;
const gui_flag_t GUI_FLAG_WINDOW                = 0x00000008;
const gui_flag_t GUI_FLAG_TOPMOST                   = 0x00000010;
// Blocks all mouse interactions with elements behind in z-order
const gui_flag_t GUI_FLAG_BLOCKING                  = 0x00000020;
// Makes the element receive OUTSIDE_MENU message if user clicked outside it's bounds
const gui_flag_t GUI_FLAG_MENU_POPUP                = 0x00000040;
const gui_flag_t GUI_FLAG_MENU_SKIP_OWNER_CLICK     = 0x00000080;
const gui_flag_t GUI_FLAG_SAME_LINE                 = 0x00000100;
const gui_flag_t GUI_FLAG_SCROLLV                   = 0x00000200;
const gui_flag_t GUI_FLAG_SCROLLH                   = 0x00000400;
// Skips layout and draw phases for element's children as if there are none
const gui_flag_t GUI_FLAG_HIDE_CONTENT              = 0x00000800;
// Skips element's body when checking for currently hovered element
// Does not skip it's children
const gui_flag_t GUI_FLAG_NO_HIT                    = 0x00001000;
// Allows to scroll content of an element by dragging the client area with left mouse button
const gui_flag_t GUI_FLAG_DRAG_CONTENT              = 0x00002000;
// Skips CLICK messages if mouse moved between button down and up
const gui_flag_t GUI_FLAG_NO_PULL_CLICK             = 0x00004000;

enum GUI_OVERFLOW {
    GUI_OVERFLOW_NONE,
    GUI_OVERFLOW_FIT
};


enum class GUI_LAYOUT {
    STACK_LEFT,
    STACK_TOP,
    FLOAT,
    FILL_H,
    FILL_V,
    FILL
};

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
        //assert(getOwner());
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
    gfxm::vec2 getClientSize() const {
        return gfxm::vec2(getClientWidth(), getClientHeight());
    }

    void layoutContentTopDown(gfxm::rect& rc) {
        if (children.empty()) {
            return;
        }
        layoutContentTopDown(rc, &children[0], children.size());
    }
    int layoutContentTopDown(gfxm::rect& rc, GuiElement** elements, int count) {
        int processed_count = 0;
        gfxm::rect& rc_ = rc;
        rc_.min += content_padding.min;
        rc_.max -= content_padding.max;
        rc_content = rc_;

        gfxm::vec2 pt_current_line = gfxm::vec2(rc_.min.x, rc_.min.y) - pos_content;
        gfxm::vec2 pt_next_line = pt_current_line;
        //rc_content = gfxm::rect(.0f, .0f, .0f, .0f);
        for(int i = 0; i < count; ++i) {
            auto& ch = elements[i];
            if (ch->hasFlags(GUI_FLAG_FLOATING)) {
                break;
            }
            if (ch->isHidden()) {
                continue;
            }
            gfxm::vec2 pos;
            gfxm::vec2 px_min_size = gui_to_px(ch->min_size, guiGetCurrentFont(), getClientSize());
            if ((ch->getFlags() & GUI_FLAG_SAME_LINE) && rc_.max.x - pt_current_line.x >= px_min_size.x) {
                pos = pt_current_line;
            } else {
                pos = pt_next_line;
                pt_current_line = pt_next_line;
            }
            float width = gui_to_px(ch->size.x, guiGetCurrentFont(), rc_.max.x - pos_content.x - pos.x);
            float height = gui_to_px(ch->size.y, guiGetCurrentFont(), rc_.max.y - pos_content.y - pos.y);
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

            ++processed_count;
        }
        rc_content.min = rc_.min - pos_content;
        rc_.max.y = rc_content.max.y;

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
        return processed_count;
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

    void sortChildren() {
        std::sort(children.begin(), children.end(), [](const GuiElement* a, const GuiElement* b)->bool {
            if (a->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING | GUI_FLAG_TOPMOST | GUI_FLAG_BLOCKING)
                == b->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING | GUI_FLAG_TOPMOST | GUI_FLAG_BLOCKING)
            ) {
                return a->getZOrder() < b->getZOrder();
            } else if(a->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING | GUI_FLAG_TOPMOST)
                == b->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING | GUI_FLAG_TOPMOST)
            ) {
                return a->checkFlags(GUI_FLAG_BLOCKING) < b->checkFlags(GUI_FLAG_BLOCKING);
            } else if(a->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING) == b->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING)) {
                return a->checkFlags(GUI_FLAG_TOPMOST) < b->checkFlags(GUI_FLAG_TOPMOST);
            } else {
                if (a->checkFlags(GUI_FLAG_FRAME)) {
                    return true;
                }
                if (b->checkFlags(GUI_FLAG_FLOATING)) {
                    return true;
                }
            }
            return false;
        });
    }
public:
    uint64_t sys_flags = 0x0;

    bool is_hidden = false;
    gui_vec2 pos = gui_vec2(100.0f, 100.0f);
    gui_vec2 size = gui_vec2(.0f, 100.0f);
    gui_vec2 min_size = gui_vec2(.0f, .0f);
    gui_vec2 max_size = gui_vec2(FLT_MAX, FLT_MAX);
    gfxm::rect content_padding = gfxm::rect(GUI_PADDING, GUI_PADDING, GUI_PADDING, GUI_PADDING);
    gfxm::rect margin = gfxm::rect(GUI_MARGIN, GUI_MARGIN, GUI_MARGIN, GUI_MARGIN);
    GUI_OVERFLOW overflow = GUI_OVERFLOW_NONE;

    const gfxm::rect& getLocalContentRect() const { return rc_content; }
    const gfxm::vec2& getLocalContentOffset() const { return pos_content; }
    void setLocalContentOffset(const gfxm::vec2& pos) { pos_content = pos; }

    int         getZOrder() const { return z_order; }
    bool        isEnabled() const { return is_enabled; }
    void        setEnabled(bool enabled) { is_enabled = enabled; }
    bool        hasFlags(gui_flag_t f) const { return (flags & f) == f; }
    gui_flag_t  checkFlags(gui_flag_t f) const { return flags & f; }
    gui_flag_t  getFlags() const { return flags; }
    void        setFlags(gui_flag_t f) { flags = f; }
    void        addFlags(gui_flag_t f) { flags = flags | f; }
    void        removeFlags(gui_flag_t f) { flags = (flags & ~f); }
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
    virtual void    setParent(GuiElement* elem) {
        if (parent) {
            parent->removeChild(this);
        }
        parent = elem;
        if (parent) {
            z_order = elem->children.size();
            elem->children.push_back(this);
            parent->sortChildren();
        }
    }

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
        sortChildren();
    }
public:
    GuiElement();
    virtual ~GuiElement();

    void setWidth(gui_float w) { size.x = w; }
    void setHeight(gui_float h) { size.y = h; }
    void setSize(gui_float w, gui_float h) { size = gui_vec2(w, h); }
    void setSize(gui_vec2 sz) { size = sz; }
    void setPosition(gui_float x, gui_float y) { pos = gui_vec2(x, y); }
    void setPosition(gui_vec2 p) { pos = p; }
    void setMinSize(gui_float w, gui_float h) { min_size = gui_vec2(w, h); }
    void setMaxSize(gui_float w, gui_float h) { max_size = gui_vec2(w, h); }
    void setHidden(bool value) { is_hidden = value; }

    bool isHovered() const;
    bool isPressed() const;
    bool isPulled() const;
    bool hasMouseCapture() const;
    bool isHidden() const { return is_hidden; }

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

    virtual void onHitTest(GuiHitResult& hit, int x, int y) {
        if (!gfxm::point_in_rect(rc_bounds, gfxm::vec2(x, y))) {
            if (hasFlags(GUI_FLAG_MENU_POPUP)) {
                hit.add(GUI_HIT::OUTSIDE_MENU, this);
            }
            return;
        }


        int i = children.size() - 1;
        // Overlapped
        for (; i >= 0; --i) {
            auto& ch = children[i];
            if (!ch->hasFlags(GUI_FLAG_FLOATING)) {
                break;
            }
            if (ch->isHidden()) {
                continue;
            }
            ch->onHitTest(hit, x, y);
            if (hit.hasHit() || ch->hasFlags(GUI_FLAG_BLOCKING)) {
                return;
            }
        }        
        // Content
        if (hasFlags(GUI_FLAG_HIDE_CONTENT)) {
            for (; i >= 0; --i) {
                auto& ch = children[i];
                if (ch->hasFlags(GUI_FLAG_FRAME)) {
                    break;
                }
            }
        } else {
            for (; i >= 0; --i) {
                auto& ch = children[i];
                if (ch->hasFlags(GUI_FLAG_FRAME)) {
                    break;
                }
                if (ch->isHidden()) {
                    continue;
                }
                ch->onHitTest(hit, x, y);
                if (hit.hasHit()) {
                    return;
                }
            }
        }
        // Frame
        for (; i >= 0; --i) {
            auto& ch = children[i];
            if (ch->isHidden()) {
                continue;
            }
            ch->onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }

        hit.add(GUI_HIT::CLIENT, this);
        return;
    }

    virtual bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
        switch (msg) {
        case GUI_MSG::CLOSE_MENU: {
            if (hasFlags(GUI_FLAG_MENU_POPUP)) {
                setHidden(true);
                return true;
            }
            break;
        }
        case GUI_MSG::RCLICK:
        case GUI_MSG::DBL_RCLICK: {
            if ((sys_flags & GUI_SYS_FLAG_HAS_CONTEXT_POPUP) != 0) {
                guiShowContextPopup(this, params.getA<int32_t>(), params.getB<int32_t>());
                return true;
            }
            break;
        }
        case GUI_MSG::PULL_START: {
            if (hasFlags(GUI_FLAG_DRAG_CONTENT)) {
                guiCaptureMouse(this);
            }
            return true;
        }
        case GUI_MSG::PULL_STOP: {
            if (hasFlags(GUI_FLAG_DRAG_CONTENT)) {
                guiCaptureMouse(0);
            }
            return true;
        }
        case GUI_MSG::PULL: {
            if (hasFlags(GUI_FLAG_DRAG_CONTENT)) {
                pos_content += gfxm::vec2(
                    params.getA<float>(), params.getB<float>()
                );
            }
            return true;
        }
        }
        return false;
    }

    virtual void onLayout(const gfxm::rect& rect, uint64_t flags) {
        rc_bounds = rect;
        client_area = rc_bounds;
        if (overflow == GUI_OVERFLOW_FIT) {
            rc_bounds.max.y = rc_bounds.min.y;
            client_area.max.y = client_area.min.y;
        }

        // Frame
        int i = 0;
        for (; i < children.size(); ++i) {
            auto& ch = children[i];
            if (!ch->hasFlags(GUI_FLAG_FRAME)) {
                break;
            }
            if (ch->isHidden()) {
                continue;
            }
            ch->layout(client_area, 0);
            client_area.min.y = ch->rc_bounds.max.y;
            client_area.max.y = gfxm::_max(client_area.max.y, client_area.min.y);
        }

        // Content
        //gfxm::expand(client_area, content_padding);
        if (!hasFlags(GUI_FLAG_HIDE_CONTENT) && children.size() > 0) {
            int count = layoutContentTopDown(client_area, &children[i], children.size() - i);
            i += count;
        } else {
            for (; i < children.size(); ++i) {
                auto& ch = children[i];
                if (ch->hasFlags(GUI_FLAG_FLOATING)) {
                    break;
                }
            }
        }

        // Overlapped
        for (; i < children.size(); ++i) {
            auto& ch = children[i];
            if (ch->isHidden()) {
                continue;
            }
            gfxm::rect rc;
            gfxm::vec2 px_min_size = gui_to_px(ch->min_size, guiGetCurrentFont(), getClientSize());
            gfxm::vec2 px_max_size = gui_to_px(ch->max_size, guiGetCurrentFont(), getClientSize());
            gfxm::vec2 px_size = gui_to_px(ch->size, guiGetCurrentFont(), getClientSize());
            gfxm::vec2 sz = gfxm::vec2(
                gfxm::_min(px_max_size.x, gfxm::_max(px_min_size.x, px_size.x)),
                gfxm::_min(px_max_size.y, gfxm::_max(px_min_size.y, px_size.y))
            );
            rc.min = rc_bounds.min + pos_content + gui_to_px(ch->pos, guiGetCurrentFont(), getClientSize());
            rc.max = rc_bounds.min + pos_content + gui_to_px(ch->pos, guiGetCurrentFont(), getClientSize()) + sz;
            ch->layout(rc, 0);
        }

        bool box_initialized = false;
        for (int i = 0; i < children.size(); ++i) {
            auto& ch = children[i];
            if (ch->hasFlags(GUI_FLAG_FRAME)) {
                continue;
            }
            if (ch->isHidden()) {
                continue;
            }
            if (!box_initialized) {
                rc_content = ch->getBoundingRect();
                box_initialized = true;
            } else {
                gfxm::expand(rc_content, ch->getBoundingRect());
            }
        }

        if (overflow == GUI_OVERFLOW_FIT) {
            gfxm::vec2 px_min_size = gui_to_px(min_size, guiGetCurrentFont(), gfxm::rect_size(rect));
            float min_max_y = gfxm::_max(rc_bounds.min.y + px_min_size.y, gfxm::_max(rc_bounds.max.y, client_area.max.y));
            rc_bounds.max.y = min_max_y;
            if (!hasFlags(GUI_FLAG_HIDE_CONTENT)) {
                rc_bounds.max.y += content_padding.max.y;
            }
        }
    }

    virtual void onDraw() {
        guiDrawPushScissorRect(rc_bounds);
        // Frame
        int i = 0;
        for (; i < children.size(); ++i) {
            auto& ch = children[i];
            if (!ch->hasFlags(GUI_FLAG_FRAME)) {
                break;
            }
            if (ch->isHidden()) {
                continue;
            }
            ch->draw();
        }

        
        // Content
        if (hasFlags(GUI_FLAG_HIDE_CONTENT)) {
            for (; i < children.size(); ++i) {
                auto& ch = children[i];
                if (ch->hasFlags(GUI_FLAG_FLOATING)) {
                    break;
                }
            }
        } else {
            guiDrawPushScissorRect(client_area);
            for (; i < children.size(); ++i) {
                auto& ch = children[i];
                if (ch->hasFlags(GUI_FLAG_FLOATING)) {
                    break;
                }
                if (ch->isHidden()) {
                    continue;
                }
                ch->draw();
            }
            guiDrawPopScissorRect();
        }

        guiDrawPopScissorRect();

        guiDrawPushScissorRect(client_area);
        // Overlapped
        for (; i < children.size(); ++i) {
            auto& ch = children[i];
            if (ch->isHidden()) {
                continue;
            }
            if (ch->hasFlags(GUI_FLAG_BLOCKING)) {
                guiDrawRect(rc_bounds, 0x77000000);
            }
            ch->draw();
        }
        guiDrawPopScissorRect();
    }

    virtual void clearChildren() {
        for (int i = 0; i < children.size(); ++i) {
            auto& ch = children[i];
            if (ch->hasFlags(GUI_FLAG_PERSISTENT)) {
                continue;
            }
            children.erase(children.begin() + i);
            --i;
        }
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
        sortChildren();
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
};