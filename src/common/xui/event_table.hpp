#pragma once

#include <stdint.h>
#include <functional>


namespace xui {

    class Element;

    struct EvtMouseMove { int lclx; int lcly; };
    struct EvtClick { int lclx; int lcly; };
    struct EvtDrag { int dx; int dy; };
    struct EvtMove { Element* subj; int dx; int dy; };
    struct EvtResizeBegin { Element* subj; gfxm::ivec2 initial_pos; gfxm::ivec2 initial_size; };
    struct EvtResize { Element* subj; gfxm::rect rc; };
    struct EvtBringToTop { Element* subj; };


    struct EventTable {
        using fn_handler_t = std::function<void(void*)>;

        struct Handler {
            uint32_t token = 0;
            std::unique_ptr<Handler> next;
            fn_handler_t fn;
        };
        std::unordered_map<std::type_index, std::unique_ptr<Handler>> handler_map;
        uint32_t next_token = 0;

        struct Token {
            std::type_index tidx;
            uint32_t token = 0;

            operator bool() const {
                return token != 0;
            }
        };

        template<typename EVT_T>
        bool invoke(EVT_T& evt) {
            std::type_index tidx = typeid(EVT_T);
            auto it = handler_map.find(tidx);
            if (it == handler_map.end()) {
                return false;
            }

            std::vector<fn_handler_t> callbacks;
            Handler* handler = it->second.get();
            while (handler) {
                callbacks.push_back(handler->fn);
                handler = handler->next.get();
            }
            for (int i = 0; i < callbacks.size(); ++i) {
                callbacks[i](&evt);
            }
            // TODO:
            return true;
        }

        template<typename EVT_T>
        Token subscribe(const std::function<void(EVT_T&)>& fn) {
            std::type_index tidx = typeid(EVT_T);
            auto it = handler_map.find(tidx);
            Handler* handler = nullptr;
            if (it == handler_map.end()) {
                it = handler_map.insert(std::make_pair(tidx, std::make_unique<Handler>())).first;
                handler = it->second.get();
            } else {
                handler = it->second.get();
                while (handler->next) {
                    handler = handler->next.get();
                }
                handler->next = std::make_unique<Handler>();
                handler = handler->next.get();
            }
            handler->fn = [fn](void* pevt) {
                fn(*static_cast<EVT_T*>(pevt));
            };
            handler->token = ++next_token;
            return Token{ tidx, handler->token };
        }

        void unsubscribe(Token tok) {
            auto it = handler_map.find(tok.tidx);
            if (it == handler_map.end()) {
                return;
            }
            if (it->second->token == tok.token) {
                it->second = std::move(it->second->next);
                if(!it->second) {
                    handler_map.erase(it);
                }
                return;
            }
            Handler* handler = it->second.get();
            while (handler) {
                Handler* next = handler->next.get();
                if (!next) {
                    break;
                }
                if (next->token == tok.token) {
                    handler->next = std::move(next->next);
                    break;
                }
                handler = handler->next.get();
            }
        }
    };

    template<typename EVT_T>
    class ScopedHandler {
        EventTable* event_table = nullptr;
        EventTable::Token token;
    public:
        ~ScopedHandler() {
            if (event_table && token) {
                event_table->unsubscribe(token);
            }
        }
        void connect(EventTable* table, const std::function<void(EVT_T&)>& fn) {
            if (event_table && token) {
                event_table->unsubscribe(token);
            }
            event_table = table;
            token = table.subscribe(fn);
        }
    };

}