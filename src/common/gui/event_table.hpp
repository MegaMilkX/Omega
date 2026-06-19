#pragma once

#include <typeindex>
#include <functional>
#include <unordered_map>


class GuiElement;

struct GuiEvt_Focus {
    mutable GuiElement* new_focused = nullptr;
    mutable bool consume = true;
};
struct GuiEvt_Unfocus {};

struct GuiEvt_MouseEnter {};
struct GuiEvt_MouseLeave {};
struct GuiEvt_MouseMove {
    int x;
    int y;
    mutable bool consume;
};
struct GuiEvt_MouseBtn {
    GUI_MOUSE_BUTTON btn;
    GUI_KEY_STATE state;
    mutable bool consume;
};
struct GuiEvt_LClick { bool is_double; int lclx; int lcly; mutable bool consume; };
struct GuiEvt_RClick { bool is_double; int lclx; int lcly; mutable bool consume; };
struct GuiEvt_MClick { bool is_double; int lclx; int lcly; mutable bool consume; };

struct GuiEvt_KeyDown {
    uint16_t vkey;
    mutable bool consume;
};
struct GuiEvt_KeyUp {
    uint16_t vkey;
    mutable bool consume;
};
struct GuiEvt_Unichar { uint32_t ch; };

struct GuiEvt_PullStart { mutable bool consume = true; };
struct GuiEvt_PullStop {};
struct GuiEvt_Pull { int dx; int dy; };


struct GuiEventTable {
    using fn_handler_t = std::function<void(const void*)>;
    struct Handler {
        std::type_index type = typeid(void);
        fn_handler_t fn = nullptr;

        template<typename EVT_T>
        bool invoke(const EVT_T& e) const {
            if (!fn) {
                return false;
            }
            if (typeid(EVT_T) != type) {
                assert(false);
                return false;
            }
            fn(&e);
            return true;
        }
    };

    std::unordered_map<std::type_index, Handler> handler_map;

    template<typename EVT_T>
    bool invoke(const EVT_T& evt) {
        std::type_index tidx = typeid(EVT_T);
        auto it = handler_map.find(tidx);
        if (it == handler_map.end()) {
            return false;
        }

        const auto& handler = it->second;
        handler.fn(&evt);
        return true;
    }

    template<typename EVT_T>
    void subscribe(const std::function<void(const EVT_T&)>& fn) {
        std::type_index tidx = typeid(EVT_T);
        auto it = handler_map.find(tidx);
        if (it == handler_map.end()) {
            it = handler_map.insert(
                std::make_pair(tidx, Handler())
            ).first;
        }

        Handler& handler = it->second;

        handler.type = tidx;
        handler.fn = [fn](const void* pevt) {
            fn(*static_cast<const EVT_T*>(pevt));
        };
    }

    template<typename EVT_T>
    Handler getHandler() {
        std::type_index tidx = typeid(EVT_T);
        auto it = handler_map.find(tidx);
        if (it == handler_map.end()) {
            return Handler { typeid(void), nullptr };
        }
        return it->second;
    }
};

