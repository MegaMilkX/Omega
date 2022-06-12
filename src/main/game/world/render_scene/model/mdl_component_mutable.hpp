#pragma once

#include <typeindex>
#include <stdint.h>

#include "reflection/reflection.hpp"

struct mdlNode;
struct mdlComponentMutable {
    TYPE_ENABLE_BASE();

    mdlNode* owner = 0;

    virtual ~mdlComponentMutable() {}

    virtual void onNodeRemoved(uint32_t n) {}
    virtual void onNodeMoved(uint32_t from, uint32_t to) {}

    virtual void reflect() = 0;
};