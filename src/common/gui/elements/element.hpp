#pragma once

#include <vector>
#include "math/gfxm.hpp"
#include "reflection/reflection.hpp"
#include "gui/gui_font.hpp"
#include "gui/gui_draw.hpp"
#include "gui/gui_values.hpp"
#include "gui/gui_color.hpp"
#include "gui/types.hpp"
#include "gui/gui_hit.hpp"
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
GuiElement* guiGetFocusedWindow();
GuiElement* guiGetActiveWindow();

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

constexpr uint32_t GUI_UTF_MAX = 0x10FFFF;
constexpr uint32_t GUI_UTF_PLACEHOLDER_0 = GUI_UTF_MAX + 1;

struct gui_layout_context {
    gfxm::vec2 extents;
    gfxm::rect frame_padding;
    uint64_t flags;
};

// TODO:
enum eGuiOwnership {
    eGuiOwnershipOwned,
    eGuiOwnershipExternal
};

class GuiHost;
class GuiElement {
    friend void guiLayout();
    friend void guiDraw();

    GuiHost* host = nullptr;
    gui_flag_t flags = 0x0;
    
    std::unique_ptr<gui::style> style;
    std::list<std::string> style_classes;
    bool needs_style_update = true;
protected:
    int linear_begin = 0;
    int linear_end = 0;
    int self_linear_size = 1;

    std::vector<GuiElement*> children;
    GuiElement* parent = 0;
    GuiElement* owner = 0;
    GuiElement* content = this;
    std::vector<std::unique_ptr<GuiElement>> owned_elements;

    gfxm::rect rc_bounds = gfxm::rect(0, 0, 0, 0);
    gfxm::rect client_area = gfxm::rect(0, 0, 0, 0);
    // Content rectangle local to the container
    gfxm::rect rc_content = gfxm::rect(.0f, .0f, .0f, .0f);
    // Content offset local to the container
    gfxm::vec2 pos_content;
    gfxm::rect tmp_padding;

    gfxm::mat4  content_view_transform_world = gfxm::mat4(1.f);
    gfxm::vec3  content_view_translation = gfxm::vec3(0, 0, 0);
    float       content_view_scale = 1.f;
public:
    uint64_t    sys_flags = 0x0;
    int         user_id = 0;
    void*       user_ptr = 0;

    bool is_hidden = false;
    bool clip_content = true;
    gui_vec2 pos = gui_vec2(100.0f, 100.0f);
    gui_vec2 size = gui_vec2(gui::perc(100), gui::content());
    gui_vec2 min_size = gui_vec2(.0f, .0f);
    gui_vec2 max_size = gui_vec2(FLT_MAX, FLT_MAX);

    gfxm::vec2 layout_position = gfxm::vec2(0, 0);

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

    struct BOX {
        GuiElement* elem;
        Font* font;
        int index;
        gui_float width;
        gui_float height;
        gui_float overflow_width;
        gui_float overflow_height;
        gui_float min_width;
        gui_float min_height;
        gui_float max_width;
        gui_float max_height;
        bool early_layout;
    };
    struct LINE {
        gui_float height;
        int begin;
        int end;
        std::vector<BOX> boxes;
    };

    std::vector<LINE> lines;

    void layoutOverlapped(int begin, uint64_t flags) {
        int i = begin;
        
        if (flags & GUI_LAYOUT_WIDTH_PASS) {
            int client_width = getClientWidth();

            for (; i < children.size(); ++i) {
                auto& ch = children[i];
                if (ch->isHidden()) {
                    continue;
                }

                Font* child_font = ch->getFont();
                gui_float width = gui_float_convert(ch->size.x, child_font, client_width);
                gui_float min_width = gui_float_convert(ch->min_size.x, child_font, client_width);
                gui_float max_width = gui_float_convert(ch->max_size.x, child_font, client_width);

                // TODO: percent
                if (width.unit == gui_content) {
                    ch->layout(
                        gfxm::vec2(0, 0),
                        GUI_LAYOUT_WIDTH_PASS | GUI_LAYOUT_FIT_CONTENT
                    );
                } else {
                    float w = gfxm::_min(max_width.value, gfxm::_max(min_width.value, width.value));
                    ch->layout(
                        gfxm::vec2(w, 0),
                        GUI_LAYOUT_WIDTH_PASS
                    );
                }
            }
        }
        if (flags & GUI_LAYOUT_HEIGHT_PASS) {
            int client_height = getClientHeight();

            for (; i < children.size(); ++i) {
                auto& ch = children[i];
                if (ch->isHidden()) {
                    continue;
                }

                Font* child_font = ch->getFont();
                gui_float height = gui_float_convert(ch->size.y, child_font, client_height);
                gui_float min_height = gui_float_convert(ch->min_size.y, child_font, client_height);
                gui_float max_height = gui_float_convert(ch->max_size.y, child_font, client_height);

                // TODO: percent
                if (height.unit == gui_content) {
                    ch->layout(
                        gfxm::vec2(0, 0),
                        GUI_LAYOUT_HEIGHT_PASS | GUI_LAYOUT_FIT_CONTENT
                    );
                } else {
                    float h = gfxm::_min(max_height.value, gfxm::_max(min_height.value, height.value));
                    ch->layout(
                        gfxm::vec2(0, h),
                        GUI_LAYOUT_HEIGHT_PASS
                    );
                }
            }
        }

        if (flags & GUI_LAYOUT_POSITION_PASS) {
            Font* font = getFont();
            for (; i < children.size(); ++i) {
                auto& ch = children[i];
                if (ch->isHidden()) {
                    continue;
                }
                ch->layout_position = pos_content + gui_to_px(ch->pos, font, getClientSize());
                ch->layout(gfxm::vec2(0, 0), GUI_LAYOUT_POSITION_PASS);
            }
        }
    }

    int layoutContentTopDown2(int begin, uint64_t flags);

    void drawContent() {
        guiDrawPushScissorRect(client_area);
        for (int i = 0; i < children.size(); ++i) {
            auto c = children[i];
            c->draw();
        }
        guiDrawPopScissorRect();
    }

    void sortChildren() {
        std::sort(children.begin(), children.end(), [](const GuiElement* a, const GuiElement* b)->bool {
            if (a->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING | GUI_FLAG_TOPMOST | GUI_FLAG_BLOCKING)
                == b->checkFlags(GUI_FLAG_FRAME | GUI_FLAG_FLOATING | GUI_FLAG_TOPMOST | GUI_FLAG_BLOCKING)
            ) {
                return false;
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
    const gfxm::rect& getLocalContentRect() const { return rc_content; }
    const gfxm::vec2& getLocalContentOffset() const { return pos_content; }
    void setLocalContentOffset(const gfxm::vec2& pos) { pos_content = pos; }

    bool        isEnabled() const { return !hasFlags(GUI_FLAG_DISABLED); }
    void        setEnabled(bool enabled) { 
        if (enabled) {
            removeFlags(GUI_FLAG_DISABLED);
        } else {
            setFlags(GUI_FLAG_DISABLED);
        }
    }
    bool        hasFlags(gui_flag_t f) const { return (flags & f) == f; }
    gui_flag_t  checkFlags(gui_flag_t f) const { return flags & f; }
    gui_flag_t  getFlags() const { return flags; }
    void        setFlags(gui_flag_t f) {
        gui_flag_t old_flags = flags;
        flags = f;
        if (old_flags != flags) {
            setStyleDirty();
        }
    }
    void        addFlags(gui_flag_t f) {
        gui_flag_t old_flags = flags;
        flags = flags | f;
        if (old_flags != flags) {
            setStyleDirty();
        }
    }
    void        removeFlags(gui_flag_t f) {
        gui_flag_t old_flags = flags;
        flags = (flags & ~f);
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
    gfxm::vec2          getGlobalPosition() const {
        if (parent) {
            return parent->getGlobalPosition() + layout_position;
        } else {
            return layout_position;
        }
    }
    gfxm::rect getGlobalBoundingRect() const {
        const gfxm::vec2 global_offs = getGlobalPosition();
        return gfxm::rect(rc_bounds.min + global_offs, rc_bounds.max + global_offs);
    }
    gfxm::rect getGlobalClientArea() const {
        const gfxm::vec2 global_offs = getGlobalPosition();
        return gfxm::rect(client_area.min + global_offs, client_area.max + global_offs);
    }
    gfxm::rect getGlobalContentRect() const {
        const gfxm::vec2 global_offs = getGlobalPosition();
        return gfxm::rect(rc_content.min + global_offs, rc_content.max + global_offs);
    }

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
            assert(false);
            return;
        }

        int at = -1;
        for (int i = 0; i < children.size(); ++i) {
            if (children[i] == e) {
                at = i;
                break;
            }
        }
        if (at == -1) {
            assert(false);
            return;
        }
        children.erase(children.begin() + at);
        children.push_back(e);
        
        sortChildren();
    }
public:
    GuiElement();
    virtual ~GuiElement();

    void _setHost(GuiHost*);
    GuiHost* getHost() const { return host; }

    void setWidth(gui_float w) { size.x = w; }
    void setHeight(gui_float h) { size.y = h; }
    void setSize(gui_float w, gui_float h) { size = gui_vec2(w, h); }
    void setSize(gui_vec2 sz) { size = sz; }
    void setPosition(gui_float x, gui_float y) { pos = gui_vec2(x, y); }
    void setPosition(gui_vec2 p) { pos = p; }
    void setMinSize(gui_float w, gui_float h) { min_size = gui_vec2(w, h); }
    void setMaxSize(gui_float w, gui_float h) { max_size = gui_vec2(w, h); }
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
    bool isFocused() const {
        //return hasFlags(GUI_FLAG_FOCUSED);
        return guiGetFocusedWindow() == this;
    }
    bool isActive() const {
        return guiGetActiveWindow() == this;
    }

    bool needsStyleUpdate() const { needs_style_update; }
    
    bool shouldDisplayScroll() const {
        // TODO: Check for GUI_OVERFLOW_SCROLL
        if (rc_content.min.y == rc_content.max.y) {
            // No content - no scrolling
            return false;
        }
        if (client_area.min.y <= rc_content.min.y && client_area.max.y >= rc_content.max.y) {
            return false;
        }
        if(gfxm::rect_size(rc_content).y <= gfxm::rect_size(client_area).y) {
            return false;
        }
        return true;
    }

    int update_selection_range(int begin);
    void apply_style();
    virtual void hitTest(GuiHitResult& hit, int x, int y);
    virtual void layout(const gfxm::vec2& extents, uint64_t flags);
    virtual void draw(int x, int y);
    void draw();

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

    virtual void onHitTest(GuiHitResult& hit, int x, int y);
    virtual bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params);
    virtual void onLayout(const gfxm::vec2& extents, uint64_t flags);
    virtual void onDraw();

    virtual void onInsertChild(GuiElement* e) {}
    virtual void onRemoveChild(GuiElement* e) {}

    virtual void clearChildren() {
        for (int i = 0; i < content->children.size(); ++i) {
            auto& ch = content->children[i];
            if (ch->hasFlags(GUI_FLAG_PERSISTENT)) {
                continue;
            }
            content->children.erase(content->children.begin() + i);
            --i;
        }
    }
    template<
        typename ELEM_T,
        std::enable_if_t<std::is_base_of<GuiElement, ELEM_T>::value>* = nullptr
    >
    ELEM_T* pushBack(ELEM_T* elem, uint64_t flags = 0) {
        elem->addFlags(flags);
        addChild(elem);
        return elem;
    }
    void pushBack(const std::string& text);
    void pushBack(const std::string& text, const std::initializer_list<std::string>& style_classes);

    // TODO: rename to addContent
    virtual void addChild(GuiElement* elem) {
        assert(content);
        content->_addChild(elem);
    }
    // TODO: rename to removeContent
    virtual void removeChild(GuiElement* elem) {
        assert(content);
        content->_removeChild(elem);
    }
    virtual void _addChild(GuiElement* elem);
    virtual void _removeChild(GuiElement* elem);
    size_t childCount() const {
        return content->children.size();
    }
    GuiElement* getChild(int i) {
        return content->children[i];
    }
    int getChildId(GuiElement* elem) {
        int id = -1;
        for (int i = 0; i < content->children.size(); ++i) {
            if (content->children[i] == elem) {
                id = i;
                break;
            }
        }
        return id;
    }
};