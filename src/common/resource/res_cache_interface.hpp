#pragma once

#include "handle/hshared.hpp"

class resCacheInterface {
public:
    virtual HSHARED_BASE* get(const char* name) = 0;
};