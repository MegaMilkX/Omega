#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "reflection/reflection.hpp"
#include "gui/gui_font.hpp"
#include "gui/gui_draw.hpp"
#include "gui/gui_values.hpp"
#include "gui/gui_color.hpp"
#include "gui/types.hpp"
#include "gui/style/styles.hpp"
#include "gui/gui_box.hpp"
#include "platform/platform.hpp"

#include "gui/gui_msg.hpp"

// TODO:
// onMeasure()
// onLayout()
// onDraw()

// forward declaration
void guiSendMessage(GuiElement* target, GUI_MSG msg, GUI_MSG_PARAMS params);
void guiCaptureMouse(GuiElement* e);
bool guiShowContextPopup(GuiElement* owner, int x, int y);

void gui_childQueuePushBack(GuiElement* e);
void gui_childQueuePushFront(GuiElement* e);
void gui_childQueuePop();
GuiElement* gui_childQueueFront();
bool gui_childQueueIsEmpty();


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

    default: return "UNKNOWN";
    }
}

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

constexpr uint32_t GUI_UTF_MAX = 0x10FFFF;
constexpr uint32_t GUI_UTF_PLACEHOLDER_0 = GUI_UTF_MAX + 1;

class GuiElement {
    friend void guiLayout();
    friend void guiDraw();

    int z_order = 0;
    bool is_enabled = true;
    gui_flag_t flags = 0x0;
    /*
    std::vector<uint32_t> inner_text_utf;*/
    
    std::unique_ptr<gui::style> style;
    std::list<std::string> style_classes;
    bool needs_style_update = true;
    //bool is_hovered = false;
protected:
    int linear_begin = 0;
    int linear_end = 0;
    int self_linear_size = 1;

    std::vector<GuiElement*> children;
    GuiElement* parent = 0;
    GuiElement* owner = 0;
    GuiElement* content = this;

    gfxm::rect rc_bounds = gfxm::rect(0, 0, 0, 0);
    gfxm::rect client_area = gfxm::rect(0, 0, 0, 0);
    // Content rectangle local to the container
    gfxm::rect rc_content = gfxm::rect(.0f, .0f, .0f, .0f);
    // Content offset local to the container
    gfxm::vec2 pos_content;

    gfxm::mat4  content_view_transform_world = gfxm::mat4(1.f);
    gfxm::vec3  content_view_translation = gfxm::vec3(0, 0, 0);
    float       content_view_scale = 1.f;
public:
    uint64_t    sys_flags = 0x0;

    bool is_hidden = false;
    gui_vec2 pos = gui_vec2(100.0f, 100.0f);
    gui_vec2 size = gui_vec2(.0f, 100.0f);
    gui_vec2 min_size = gui_vec2(.0f, .0f);
    gui_vec2 max_size = gui_vec2(FLT_MAX, FLT_MAX);
    GUI_OVERFLOW overflow = GUI_OVERFLOW_NONE;

protected:
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


    void layoutFitSelf(const gfxm::rect& rc) {
        gfxm::vec2 px_enclosing_sz = gfxm::rect_size(rc);
        gfxm::vec2 px_size = gui_to_px(size, getFont(), px_enclosing_sz);
        if (px_size.x == .0f) { px_size.x = px_enclosing_sz.x; }
        if (px_size.y == .0f) { px_size.y = px_enclosing_sz.y; }
        rc_bounds.min = rc.min;
        rc_bounds.max = rc.min + px_size;
        client_area = rc_bounds;
        // TODO: padding
    }
    void layoutListChildrenBelow() {
        float x = rc_bounds.min.x;
        float y = rc_bounds.max.y;
        for (int i = 0; i < children.size(); ++i) {
            auto& ch = children[i];
            gfxm::rect rc;
            rc.min = gfxm::vec2(x, y);
            rc.max.x = rc_bounds.max.x;
            rc.max.y = rc.min.y + 30.f;
            ch->layout(rc, 0);
            y += gfxm::rect_size(ch->getBoundingRect()).y;
        }
    }
    void layoutFitBoundsToChildren() {
        for (int i = 0; i < children.size(); ++i) {
            auto& ch = children[i];
            gfxm::expand(rc_bounds, ch->getBoundingRect());
        }
    }
    void layoutContentTopDown(gfxm::rect& rc) {
        if (children.empty()) {
            return;
        }
        layoutContentTopDown(rc, 0);
    }
    int layoutContentTopDown(gfxm::rect& rc, int start_at) {
        int processed_count = 0;

        Font* font = getFont();
        auto style_border = getStyleComponent<gui::style_border>();
        auto box_style = getStyleComponent<gui::style_box>();

        gui_rect gui_padding;
        gfxm::rect px_padding;
        if (box_style) {
            gui_padding = box_style->padding.has_value() ? box_style->padding.value() : gui_rect();
        }
        px_padding = gui_to_px(gui_padding, font, getClientSize());
        gfxm::rect& rc_ = rc;
        rc_.min += px_padding.min;
        rc_.max -= px_padding.max;
        if (style_border) {
            rc_.min += gfxm::vec2(
                gui_to_px(style_border->thickness_left.value(), font, getClientSize().x),
                gui_to_px(style_border->thickness_top.value(), font, getClientSize().y)
            );
            rc_.max -= gfxm::vec2(
                gui_to_px(style_border->thickness_right.value(), font, getClientSize().x),
                gui_to_px(style_border->thickness_bottom.value(), font, getClientSize().y)
            );
        }

        rc_content = rc_;

        gfxm::vec2 pt_current_line = gfxm::vec2(rc_.min.x, rc_.min.y) - pos_content;
        gfxm::vec2 pt_next_line = pt_current_line;
        float prev_bottom_margin = .0f;
        for(int i = start_at; i < children.size(); ++i) {
            auto ch = children[i];
            if (ch->hasFlags(GUI_FLAG_FLOATING)) {
                break;
            }
            if (ch->isHidden()) {
                ++processed_count;
                continue;
            }
            Font* child_font = ch->getFont();

            gui_rect gui_margin;

            auto box_style = ch->getStyleComponent<gui::style_box>();
            
            if (box_style) {
                gui_margin = box_style->margin.has_value() ? box_style->margin.value() : gui_rect();
            }
            
            gfxm::rect px_margin = gui_to_px(gui_margin, child_font, getClientSize());

            gfxm::vec2 pos;
            gfxm::vec2 px_min_size = gui_to_px(ch->min_size, child_font, getClientSize());
            float top_margin = px_margin.min.y;
            top_margin = gfxm::_max(prev_bottom_margin, top_margin);
            if ((ch->getFlags() & GUI_FLAG_SAME_LINE) && rc_.max.x - pt_current_line.x >= px_min_size.x) {
                pos = pt_current_line + gfxm::vec2(0, top_margin);
            } else {
                pos = pt_next_line + gfxm::vec2(0, top_margin);
                pt_current_line = pt_next_line;
            }
            float width = gui_to_px(ch->size.x, child_font, rc_.max.x - pos_content.x - pos.x);
            float height = gui_to_px(ch->size.y, child_font, rc_.max.y - pos_content.y - pos.y);
            if (width == .0f) {
                width = gfxm::_max(.0f, rc_.max.x - pos_content.x - pos.x);
            }
            if (height == .0f) {
                height = gfxm::_max(.0f, rc_.max.y - pos_content.y - pos.y);
            }
            gfxm::rect rect(pos, pos + gfxm::vec2(width, height));

            ch->layout(rect, 0);
            pt_next_line = gfxm::vec2(rc_.min.x - pos_content.x, gfxm::_max(pt_next_line.y - pos_content.y, ch->getBoundingRect().max.y));
            pt_current_line.x = ch->getBoundingRect().max.x;
            prev_bottom_margin = px_margin.max.y;

            rc_content.max.y = gfxm::_max(rc_content.max.y, ch->getBoundingRect().max.y + px_margin.min.y);
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

    virtual void onReportChildren() {
        for (auto ch : children) {
            gui_childQueuePushBack(ch);
        }
    }
public:
    const gfxm::rect& getLocalContentRect() const { return rc_content; }
    const gfxm::vec2& getLocalContentOffset() const { return pos_content; }
    void setLocalContentOffset(const gfxm::vec2& pos) { pos_content = pos; }

    int         getZOrder() const { return z_order; }
    bool        isEnabled() const { return is_enabled; }
    void        setEnabled(bool enabled) { is_enabled = enabled; }
    bool        hasFlags(gui_flag_t f) const { return (flags & f) == f; }
    gui_flag_t  checkFlags(gui_flag_t f) const { return flags & f; }
    gui_flag_t  getFlags() const { return flags; }
    void        setFlags(gui_flag_t f) {
        gui_flag_t old_flags = flags;
        flags = f; /*box.setFlags(f);*/
        if (old_flags != flags) {
            setStyleDirty();
        }
    }
    void        addFlags(gui_flag_t f) {
        gui_flag_t old_flags = flags;
        flags = flags | f; /*box.addFlags(f);*/
        if (old_flags != flags) {
            setStyleDirty();
        }
    }
    void        removeFlags(gui_flag_t f) {
        gui_flag_t old_flags = flags;
        flags = (flags & ~f); /*box.removeFlags(f);*/
        if (old_flags != flags) {
            setStyleDirty();
        }
    }
    Font*       getFont() { 
        if (!style) {
            return guiGetDefaultFont();
        }
        auto font_style = style->get_component<gui::style_font>();
        if (!font_style || font_style->font == nullptr) {
            return guiGetDefaultFont();
        }
        return font_style->font.get();
    }

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
    virtual void    setParent(GuiElement* elem);

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

    void setWidth(gui_float w) { size.x = w; /*box.setWidth(w);*/ }
    void setHeight(gui_float h) { size.y = h; /*box.setHeight(h);*/ }
    void setSize(gui_float w, gui_float h) { size = gui_vec2(w, h); /*box.setSize(w, h);*/ }
    void setSize(gui_vec2 sz) { size = sz; /*box.setSize(sz);*/ }
    void setPosition(gui_float x, gui_float y) { pos = gui_vec2(x, y); /*box.setPosition(x, y);*/ }
    void setPosition(gui_vec2 p) { pos = p; /*box.setPosition(p);*/ }
    void setMinSize(gui_float w, gui_float h) { min_size = gui_vec2(w, h); /*box.setMinSize(w, h);*/ }
    void setMaxSize(gui_float w, gui_float h) { max_size = gui_vec2(w, h); /*box.setMaxSize(w, h);*/ }
    void setHidden(bool value) { is_hidden = value; }
    void setSelected(bool value) { value ? addFlags(GUI_FLAG_SELECTED) : removeFlags(GUI_FLAG_SELECTED); }

    void setStyleDirty() {
        needs_style_update = true;
        for (auto& ch : children) {
            ch->setStyleDirty();
        }
    }

    void setStyleClasses(const std::initializer_list<std::string>& list) {
        style_classes = list;
        needs_style_update = true;
    }
    void setStyleClasses(const std::list<std::string>& list) {
        style_classes = list;
        needs_style_update = true;
    }
    const std::list<std::string>& getStyleClasses() const {
        return style_classes;
    }

    gui::style* getStyle() {
        if (style == nullptr) {
            style.reset(new gui::style);
        }
        return style.get();
    }

    template<typename T>
    bool hasStyleComponent() { return getStyle()->has_component<T>(); }
    template<typename T>
    T* getStyleComponent() { return getStyle()->get_component<T>(); }
    template<typename T>
    GuiElement* addStyleComponent(const T& style_) {
        getStyle()->add_component(style_);
        return this;
    }
    template<typename T>
    T* addStyleComponent() { return getStyle()->add_component<T>(); }

    bool isHovered() const;
    bool isPressed() const;
    bool isPulled() const;
    bool hasMouseCapture() const;
    bool isHidden() const { return is_hidden; }
    bool isSelected() const { return hasFlags(GUI_FLAG_SELECTED); }

    bool needsStyleUpdate() const { needs_style_update; }

    int update_selection_range(int begin);
    void apply_style();
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
        case GUI_MSG::MOUSE_ENTER: {
            //is_hovered = true;
            setStyleDirty();
            return false;
        }
        case GUI_MSG::MOUSE_LEAVE: {
            //is_hovered = false;
            setStyleDirty();
            return false;
        }
        case GUI_MSG::LBUTTON_DOWN:
        case GUI_MSG::LBUTTON_UP:
            setStyleDirty();
            return false;
        case GUI_MSG::CLOSE_MENU: {
            if (hasFlags(GUI_FLAG_MENU_POPUP)) {
                setHidden(true);
                return true;
            }
            break;
        }
        case GUI_MSG::LCLICK: {
            if (hasFlags(GUI_FLAG_SELECTABLE)) {
                if (!hasFlags(GUI_FLAG_SELECTED)) {
                    addFlags(GUI_FLAG_SELECTED);
                } else {
                    removeFlags(GUI_FLAG_SELECTED);
                }
            }
            return false;
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

        Font* font = getFont();

        auto box_style = getStyleComponent<gui::style_box>();
        gui_rect gui_padding;
        gfxm::rect px_padding;
        if (box_style) {
            gui_padding = box_style->padding.has_value() ? box_style->padding.value() : gui_rect();
        }
        px_padding = gui_to_px(gui_padding, font, getClientSize());

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
        //gfxm::expand(client_area, padding);
        if (!hasFlags(GUI_FLAG_HIDE_CONTENT) && children.size() > 0) {
            int count = layoutContentTopDown(client_area, i);
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
            Font* child_font = ch->getFont();
            gfxm::rect rc;
            gfxm::vec2 px_min_size = gui_to_px(ch->min_size, child_font, getClientSize());
            gfxm::vec2 px_max_size = gui_to_px(ch->max_size, child_font, getClientSize());
            gfxm::vec2 px_size = gui_to_px(ch->size, child_font, getClientSize());
            gfxm::vec2 sz = gfxm::vec2(
                gfxm::_min(px_max_size.x, gfxm::_max(px_min_size.x, px_size.x)),
                gfxm::_min(px_max_size.y, gfxm::_max(px_min_size.y, px_size.y))
            );
            rc.min = rc_bounds.min + pos_content + gui_to_px(ch->pos, font, getClientSize());
            rc.max = rc_bounds.min + pos_content + gui_to_px(ch->pos, font, getClientSize()) + sz;
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
            gfxm::vec2 px_min_size = gui_to_px(min_size, font, gfxm::rect_size(rect));
            float min_max_y = gfxm::_max(rc_bounds.min.y + px_min_size.y, gfxm::_max(rc_bounds.max.y, client_area.max.y));
            rc_bounds.max.y = min_max_y;
            if (!hasFlags(GUI_FLAG_HIDE_CONTENT)) {
                rc_bounds.max.y += px_padding.max.y;
            }
        }
    }

    virtual void onDraw() {
        // Self
        {
            Font* font = getFont();

            GuiElement* e = this;
            gfxm::rect rc = e->getBoundingRect();
            float rc_width = rc.max.x - rc.min.x;
            float rc_height = rc.max.y - rc.min.y;
            float min_side = gfxm::_min(rc_width, rc_height);
            float half_min_side = min_side * .5f;
            auto bg_color_style = e->getStyleComponent<gui::style_background_color>();
            auto border_style = e->getStyleComponent<gui::style_border>();
            auto border_radius_style = e->getStyleComponent<gui::style_border_radius>();

            if (bg_color_style) {
                if (border_radius_style) {
                    float br_tl = gui_to_px(border_radius_style->radius_top_left.value(), font, half_min_side);
                    float br_tr = gui_to_px(border_radius_style->radius_top_right.value(), font, half_min_side);
                    float br_bl = gui_to_px(border_radius_style->radius_bottom_left.value(), font, half_min_side);
                    float br_br = gui_to_px(border_radius_style->radius_bottom_right.value(), font, half_min_side);
                    guiDrawRectRound(
                        rc, br_tl, br_tr, br_bl, br_br,
                        bg_color_style->color.value()
                    );
                } else {
                    guiDrawRect(rc, bg_color_style->color.value());
                }
            }
            if (border_style) {
                if (border_radius_style) {
                    float br_tl = gui_to_px(border_radius_style->radius_top_left.value(), font, half_min_side);
                    float br_tr = gui_to_px(border_radius_style->radius_top_right.value(), font, half_min_side);
                    float br_bl = gui_to_px(border_radius_style->radius_bottom_left.value(), font, half_min_side);
                    float br_br = gui_to_px(border_radius_style->radius_bottom_right.value(), font, half_min_side);
                    float bt_l = gui_to_px(border_style->thickness_left.value(), font, rc_width * .5f);
                    float bt_t = gui_to_px(border_style->thickness_top.value(), font, rc_height * .5f);
                    float bt_r = gui_to_px(border_style->thickness_right.value(), font, rc_width * .5f);
                    float bt_b = gui_to_px(border_style->thickness_bottom.value(), font, rc_height * .5f);
                    guiDrawRectRoundBorder(
                        rc, br_tl, br_tr, br_bl, br_br,
                        bt_l, bt_t, bt_r, bt_b,
                        border_style->color_left.value(), border_style->color_top.value(), border_style->color_right.value(), border_style->color_bottom.value()
                    );
                } else {
                    // TODO:
                }
            }
        }
        // ----

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
    void pushBack(GuiElement* elem) {
        addChild(elem);
    }
    void pushBack(const std::string& text);
    void pushBack(const std::string& text, const std::initializer_list<std::string>& style_classes);
    void pushFront(GuiElement* elem) {
        int z = 0;
        if (!children.empty()) {
            z = children[0]->getZOrder() - 1;
        }
        addChild(elem);
        elem->z_order = z;
    }
    void _insertAfter(GuiElement* elem, GuiElement* inserted);
    void _erase(GuiElement* erased);
    virtual void addChild(GuiElement* elem) {
        assert(content);
        content->_addChild(elem);
    }
    virtual void removeChild(GuiElement* elem) {
        assert(content);
        content->_removeChild(elem);
    }
    virtual void _addChild(GuiElement* elem);
    virtual void _removeChild(GuiElement* elem);
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