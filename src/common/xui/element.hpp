#pragma once

#include <list>
#include <memory>

#include "math/gfxm.hpp"
#include "typeface/font.hpp"
#include "xui/renderer/renderer.hpp"
#include "xui/types.hpp"
#include "xui/hit.hpp"
#include "xui/layout_context.hpp"

#include "xui/layout/layout_handler.hpp"
#include "xui/event_table.hpp"

#include "gui/style/style_component.hpp"


namespace xui {


    enum MouseButton {
        MouseLeft,
        MouseRight,
        MouseMid,
        MouseScroll
    };
    enum KeyEvent {
        KeyNone,
        KeyDown,
        KeyUp,
    };

    typedef uint8_t behavior_flags_t;
    constexpr behavior_flags_t BHV_FLAG_CAPTURE_ON_MOUSE_DOWN   = 0x01;
    constexpr behavior_flags_t BHV_FLAG_UNUSED1                 = 0x02;
    constexpr behavior_flags_t BHV_FLAG_UNUSED2                 = 0x04;

    typedef uint8_t display_flags_t;
    constexpr display_flags_t DISPLAY_FLAG_HOVERED  = 0x01;
    constexpr display_flags_t DISPLAY_FLAG_PRESSED  = 0x02;
    constexpr display_flags_t DISPLAY_FLAG_SELECTED = 0x04;
    constexpr display_flags_t DISPLAY_FLAG_FOCUSED  = 0x08;
    constexpr display_flags_t DISPLAY_FLAG_ACTIVE   = 0x10;
    constexpr display_flags_t DISPLAY_FLAG_DISABLED = 0x20;
    constexpr display_flags_t DISPLAY_FLAG_READONLY = 0x40;

    typedef uint8_t draw_flags_t;
    constexpr draw_flags_t DRAW_FLAG_CLIP_CONTENT = 0x01;

    class Host;
    class Element {
        friend Host;

        int ref_count = 0;

        void addRef() { ++ref_count; }
        void releaseRef() { --ref_count; }

    protected:
        void registerChild(Element*);
        void unregisterChild(Element*);

        void setContentTarget(Element*);
    public:
        behavior_flags_t behavior_flags = 0;
        display_flags_t display_flags = 0;
        draw_flags_t    draw_flags = 0;
        bool is_hidden = false; // TODO: should be one of the flags

        Element* layout_parent = nullptr;
        // GuiBox
        std::vector<Element*> layout_children;
        Element* content_target = this;

        LayoutMode layout_mode = LayoutBox;
        std::unique_ptr<LayoutHandler> layout_handler;
        LAYOUT_PHASE next_layout_phase = LAYOUT_PHASE::WIDTH;
        std::vector<std::unique_ptr<Element>> owned_children;
        //

        //
        HIT hit_type = HIT::CLIENT;
        //

        //
        std::unique_ptr<gui::style> style;
        std::list<std::string> style_selectors;
        Font* resolved_font = nullptr;
        bool is_style_dirty = true;
        //

        //
        gui_vec2 size = gui::px(100, 100);
        gui_vec2 min_size = gui::px(0, 0);
        gui_vec2 max_size = gui::px(INT_MAX, INT_MAX);
        bool same_line = false;
        //

        gfxm::ivec2 px_pos; // in parent space
        gfxm::ivec2 px_size;
        gfxm::ivec2 px_min_size;

        std::unique_ptr<EventTable> event_table;


        virtual ~Element();

        virtual void onHitTest(HitResult& hit, int x, int y);
        virtual void onLayout(Host*, LayoutContext&);
        virtual void layout(Host*, LayoutContext&);
        virtual void onDraw(IRenderer*);

        void applyStyle(Host*);
        void hitTest(HitResult& hit, int x, int y);
        void draw(IRenderer*);

        void addToLayout(Element* e); // TODO: Think this through
        template<typename ELEM_T, typename... ARGS>
        ELEM_T* createItem(ARGS&&... args);

        template<typename EVT_T>
        EventTable::Token subscribe(const std::function<void(EVT_T&)>& fn);
        void unsubscribe(EventTable::Token tok);
        template<typename EVT_T>
        bool invoke(const EVT_T&);
        template<typename EVT_T>
        bool invokeMutable(EVT_T&);

        gfxm::ivec2 globalToLocal(const gfxm::ivec2&) const;
        gfxm::ivec2 getGlobalPos() const;
        gfxm::rect getGlobalRect() const;
        gui::style* getStyle();
        Element* getContentTarget();

        void setHidden(bool value);
        bool isHidden() const;

        void setBehaviorFlags(behavior_flags_t flags);
        void clearBehaviorFlags(behavior_flags_t flags);

        void setDisplayFlags(display_flags_t flags);
        void clearDisplayFlags(display_flags_t flags);

        bool isHovered() const { return display_flags & DISPLAY_FLAG_HOVERED; }
        bool isPressed() const { return display_flags & DISPLAY_FLAG_PRESSED; }
        bool isSelected() const { return display_flags & DISPLAY_FLAG_SELECTED; }
        bool isFocused() const { return display_flags & DISPLAY_FLAG_FOCUSED; }
        bool isActive() const { return display_flags & DISPLAY_FLAG_ACTIVE; }
        bool isDisabled() const { return display_flags & DISPLAY_FLAG_DISABLED; }
        bool isReadOnly() const { return display_flags & DISPLAY_FLAG_READONLY; }
    };

    template<typename ELEM_T, typename... ARGS>
    ELEM_T* Element::createItem(ARGS&&... args) {
        ELEM_T* ptr = new ELEM_T(std::forward<ARGS>(args)...);
        owned_children.push_back(std::unique_ptr<Element>(ptr));
        registerChild(ptr);
        return ptr;
    }

    template<typename EVT_T>
    EventTable::Token Element::subscribe(const std::function<void(EVT_T&)>& fn) {
        if (!event_table) {
            event_table = std::make_unique<EventTable>();
        }
        return event_table->subscribe(fn);
    }
    inline void Element::unsubscribe(EventTable::Token tok) {
        if (!event_table) {
            return;
        }
        event_table->unsubscribe(tok);
    }
    template<typename EVT_T>
    bool Element::invoke(const EVT_T& e) {
        if (!event_table) {
            return false;
        }
        EVT_T copy = e;
        return event_table->invoke(copy);
    }
    template<typename EVT_T>
    bool Element::invokeMutable(EVT_T& e) {
        if (!event_table) {
            return false;
        }
        return event_table->invoke(e);
    }


}

