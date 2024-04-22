#pragma once

#include <queue>
#include <stack>
#include <string>
#include <functional>

// A simple utility for automatic cleanup calls in correct order

typedef std::function<bool(void)> init_handler_init_fn_t;
typedef std::function<void(void)> init_handler_cleanup_fn_t;

struct InitHandlerPair {
    std::string name; // For logging
    init_handler_init_fn_t init_fn;
    init_handler_cleanup_fn_t cleanup_fn;
};

class InitHandlerRAII {
    bool initialized = false;
    std::stack<init_handler_cleanup_fn_t> cleanup_stack;
    std::queue<InitHandlerPair> pair_queue;
public:
    InitHandlerRAII() {}
    ~InitHandlerRAII();
    InitHandlerRAII& add(const char* name, init_handler_init_fn_t init_fn, init_handler_cleanup_fn_t cleanup_fn);
    bool init();
};
