#pragma once

#include <typeindex>
#include <functional>
#include <unordered_map>


struct GuiEvt_MouseBtn {
    GUI_MOUSE_BUTTON btn;
    GUI_KEY_STATE state;
    mutable bool consume;
};
struct GuiEvt_LClick { bool is_double; int lclx; int lcly; };
struct GuiEvt_RClick { bool is_double; int lclx; int lcly; };
struct GuiEvt_MClick { bool is_double; int lclx; int lcly; };


struct GuiEventHandlerToken {
    std::type_index tidx;
    uint32_t token = 0;

    operator bool() const {
        return token != 0;
    }
};


struct GuiEventTable {
    uint32_t next_token = 0;
    using fn_handler_t = std::function<void(const void*)>;
    struct Handler {
        uint32_t token = 0;
        fn_handler_t fn = nullptr;
    };

    std::unordered_map<std::type_index, std::vector<Handler>> handler_map;

    template<typename EVT_T>
    bool invoke(const EVT_T& evt) {
        std::type_index tidx = typeid(EVT_T);
        auto it = handler_map.find(tidx);
        if (it == handler_map.end()) {
            return false;
        }

        const auto& handlers = it->second;

        std::vector<fn_handler_t> callbacks_copy(handlers.size());
        for (int i = 0; i < handlers.size(); ++i) {
            callbacks_copy[i] = handlers[i].fn;
        }
        for (int i = 0; i < callbacks_copy.size(); ++i) {
            callbacks_copy[i](&evt);
        }
        return true;
    }

    template<typename EVT_T>
    GuiEventHandlerToken subscribe(const std::function<void(const EVT_T&)>& fn) {
        std::type_index tidx = typeid(EVT_T);
        auto it = handler_map.find(tidx);
        if (it == handler_map.end()) {
            it = handler_map.insert(
                std::make_pair(tidx, std::vector<Handler>())
            ).first;
        }

        Handler& handler = it->second.emplace_back();

        handler.fn = [fn](const void* pevt) {
            fn(*static_cast<const EVT_T*>(pevt));
        };
        handler.token = ++next_token;
        return GuiEventHandlerToken{ tidx, handler.token };
    }

    void unsubscribe(GuiEventHandlerToken tok) {
        auto it = handler_map.find(tok.tidx);
        if (it == handler_map.end()) {
            return;
        }

        auto& vec = it->second;
        for (int i = 0; i < vec.size(); ++i) {
            auto& h = vec[i];
            if (h.token == tok.token) {
                vec.erase(vec.begin() + i);
                break;
            }
        }
    }
};

