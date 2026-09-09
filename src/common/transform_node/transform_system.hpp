#pragma once

#include <stdint.h>
#include <memory>


typedef void (*pfn_transform_callback_t)(void*);

struct TransformCallback {
    pfn_transform_callback_t callback;
    std::unique_ptr<TransformCallback> next;
    void* context;
    int id;

    void call() {
        callback(context);
        if (next) {
            next->callback(next->context);
        }
    }
};


class TransformSystem {
    static uint32_t generation;
public:
    static void nextFrame();
    static uint32_t getGeneration();

    static void scheduleCallback(TransformCallback*);
    static void cancelCallback(TransformCallback*);
};