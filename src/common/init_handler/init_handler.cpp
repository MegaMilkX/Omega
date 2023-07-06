#include "init_handler.hpp"
#include <assert.h>
#include "log/log.hpp"


InitHandlerRAII::~InitHandlerRAII() {
    while (!cleanup_stack.empty()) {
        auto cleanup_fn = cleanup_stack.top();
        cleanup_stack.pop();
        if (cleanup_fn == 0) {
            continue;
        }
        cleanup_fn();
    }
}

InitHandlerRAII& InitHandlerRAII::add(const char* name, init_handler_init_fn_t init_fn, init_handler_cleanup_fn_t cleanup_fn) {
    pair_queue.push(
        InitHandlerPair{ name, init_fn, cleanup_fn }
    );
    return *this;
}

bool InitHandlerRAII::init() {
    while (!pair_queue.empty()) {
        auto pair = pair_queue.front();
        pair_queue.pop();

        LOG("'" << pair.name << "' initializing...");
        bool result = pair.init_fn();
        if (!result) {
            LOG_ERR("Failed to initialize '" << pair.name << "'");
            assert(false);
            return false;
        }
        cleanup_stack.push(pair.cleanup_fn);
        LOG("'" << pair.name << "' done.");
    }
    return true;
}