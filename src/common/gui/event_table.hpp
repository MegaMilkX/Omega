#pragma once

#include <typeindex>
#include <functional>
#include <unordered_map>


class GuiElement;
struct GuiEventHandler;

struct GuiEvent {
    mutable std::function<bool(void)> next_fn = nullptr;
    mutable bool consume = true;

    virtual ~GuiEvent() {}
    bool invoke_next() const;
};

#define DEF_GUI_EVENT(NAME) \
struct GuiEvt_ ## NAME : public GuiEvent

struct GuiEvt_Focus : public GuiEvent {
    GuiEvt_Focus(GuiElement* new_focused) : new_focused(new_focused) {}
    mutable GuiElement* new_focused = nullptr;
};
struct GuiEvt_Unfocus : public GuiEvent {};

struct GuiEvt_Selected : public GuiEvent {
    GuiEvt_Selected(GuiElement* elem) : elem(elem) {}
    GuiElement* elem = nullptr;
};
struct GuiEvt_Deselected : public GuiEvent {
    GuiEvt_Deselected(GuiElement* elem) : elem(elem) {}
    GuiElement* elem = nullptr;
}; // TODO: not in use right now

struct GuiEvt_MouseEnter : public GuiEvent {};
struct GuiEvt_MouseLeave : public GuiEvent {};
struct GuiEvt_MouseMove : public GuiEvent {
    GuiEvt_MouseMove(int x, int y) : x(x), y(y) {}
    int x; int y;
};
struct GuiEvt_MouseBtn : public GuiEvent {
    GuiEvt_MouseBtn(GUI_MOUSE_BUTTON btn, GUI_KEY_STATE state) : btn(btn), state(state) {}
    GUI_MOUSE_BUTTON btn; GUI_KEY_STATE state;
};
struct GuiEvt_LClick : public GuiEvent {
    GuiEvt_LClick(bool is_double, int lclx, int lcly) : is_double(is_double), lclx(lclx), lcly(lcly) {}
    bool is_double; int lclx; int lcly;
};
struct GuiEvt_RClick : public GuiEvent {
    GuiEvt_RClick(bool is_double, int lclx, int lcly) : is_double(is_double), lclx(lclx), lcly(lcly) {}
    bool is_double; int lclx; int lcly;
};
struct GuiEvt_MClick : public GuiEvent {
    GuiEvt_MClick(bool is_double, int lclx, int lcly) : is_double(is_double), lclx(lclx), lcly(lcly) {}
    bool is_double; int lclx; int lcly;
};

struct GuiEvt_KeyDown : public GuiEvent {
    GuiEvt_KeyDown(uint16_t vkey) : vkey(vkey) {}
    uint16_t vkey;
};
struct GuiEvt_KeyUp : public GuiEvent {
    GuiEvt_KeyUp(uint16_t vkey) : vkey(vkey) {}
    uint16_t vkey;
};
struct GuiEvt_Unichar : public GuiEvent {
    GuiEvt_Unichar(uint32_t ch) : ch(ch) {}
    uint32_t ch;
};

struct GuiEvt_PullStart : public GuiEvent {};
struct GuiEvt_PullStop : public GuiEvent {};
struct GuiEvt_Pull : public GuiEvent {
    GuiEvt_Pull(int dx, int dy) : dx(dx), dy(dy) {}
    int dx;
    int dy;
};


struct GuiEventHandler {
    using fn_handler_t = std::function<void(const void*)>;

    std::type_index type = typeid(void);
    fn_handler_t fn = nullptr;
    std::unique_ptr<GuiEventHandler> next;

    GuiEventHandler() {}
    GuiEventHandler(const GuiEventHandler& other) {
        type = other.type;
        fn = other.fn;
        if (other.next) {
            next.reset(new GuiEventHandler);
            next->type = other.type;
            next->fn = other.fn;
        } else {
            next.reset();
        }
    }
    GuiEventHandler(GuiEventHandler&& other) {
        type = other.type;
        fn = other.fn;
        next = std::move(other.next);
    }

    template<typename EVT_T>
    bool invoke(const EVT_T& e) const {
        static_assert(std::is_base_of_v<GuiEvent, EVT_T>, "Gui event must be derived from GuiEvent");

        if (!fn) {
            return false;
        }
        if (typeid(EVT_T) != type) {
            assert(false);
            return false;
        }
        if(next) {
            auto next_ptr = next.get();
            e.next_fn = [next_ptr, &e]()->bool{ return next_ptr->invoke(e); };
        } else {
            e.next_fn = nullptr;
        }
        fn(&e);
        return true;
    }
};

inline bool GuiEvent::invoke_next() const {
    if(!next_fn) return false;
    return next_fn();
}

struct GuiEventTable {
    std::unordered_map<std::type_index, GuiEventHandler> handler_map;

    template<typename EVT_T>
    bool invoke(const EVT_T& evt) {
        std::type_index tidx = typeid(EVT_T);
        auto it = handler_map.find(tidx);
        if (it == handler_map.end()) {
            return false;
        }

        const auto& handler = it->second;
        return handler.invoke(evt);
    }

    template<typename EVT_T>
    void subscribe(const std::function<void(const EVT_T&)>& fn) {
        std::type_index tidx = typeid(EVT_T);
        auto it = handler_map.find(tidx);
        if (it == handler_map.end()) {
            it = handler_map.insert(
                std::make_pair(tidx, GuiEventHandler())
            ).first;
            
            GuiEventHandler& handler = it->second;
            handler.type = tidx;
            handler.fn = [fn](const void* pevt) {
                fn(*static_cast<const EVT_T*>(pevt));
            };
        } else {
            GuiEventHandler& handler = it->second;
            handler.next.reset(new GuiEventHandler());
            handler.next->type = handler.type;
            handler.next->fn = handler.fn;
            handler.type = tidx;
            handler.fn = [fn](const void* pevt) {
                fn(*static_cast<const EVT_T*>(pevt));
            };
        }
    }

    template<typename EVT_T>
    GuiEventHandler getHandler() {
        std::type_index tidx = typeid(EVT_T);
        auto it = handler_map.find(tidx);
        if (it == handler_map.end()) {
            return GuiEventHandler();
        }
        return it->second;
    }
};

