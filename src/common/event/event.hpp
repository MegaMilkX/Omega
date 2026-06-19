#pragma once

#include <vector>
#include <utility>
#include <functional>


template<typename... Args>
class Event {
public:
    using handler_t = std::function<void(Args...)>;
    using id_t = uint32_t;

    id_t subscribe(handler_t hdl) {
        if(next_id == 0) next_id = 1;
        id_t id = next_id++;
        handlers.push_back(std::make_pair(id, std::move(hdl)));
        return id;
    }
    void unsubscribe(id_t id) {
        std::erase_if(handlers, [id](const auto& pair) { return pair.first == id; });
    }

    void invoke(Args... args) const {
        std::vector<std::pair<id_t, handler_t>> copy = handlers;
        for (auto& [id, handler] : copy) {
            handler(args...);
        }
    }

    void invoke_one(id_t handler_id, Args... args) const {
        for (auto& [id, handler] : handlers) {
            if (id != handler_id) {
                continue;
            }
            handler(args...);
        }
    }

private:
    id_t next_id = 1;
    std::vector<std::pair<id_t, handler_t>> handlers;
};

