#pragma once

#include "math/gfxm.hpp"
#include <algorithm>


inline void guiTimelineCalcDividers(float frame_width, int& prime, int& mid, int& skip) {
    int id = (100.0f / frame_width) / 5;
    int div = std::max(5, id * 5);
    prime = div;
    if ((div % 2) == 0) {
        mid = std::max(1, div / 2);
    }
    else {
        mid = 0;
    }
    if (mid != 0) {
        skip = std::max(1, div / 5);// std::max(1.0f, 10.0f / frame_screen_width);
    }
    else {
        skip = std::max(1, div / 5);
    }
}

