#pragma once

#include "common/math/gfxm.hpp"

class GuiIDrawInterface {
public:
    virtual ~GuiIDrawInterface() {}

    virtual void drawRect(const gfxm::rect& rc) = 0;
    virtual void drawLines(const gfxm::vec3* vertices, int count) = 0;
};