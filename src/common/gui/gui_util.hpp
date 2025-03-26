#pragma once

#include "math/gfxm.hpp"


void guiCalcResizeBorders(
    const gfxm::rect& rect,
    float thickness_outer, float thickness_inner,
    gfxm::rect* left, gfxm::rect* right, gfxm::rect* top, gfxm::rect* bottom
);

