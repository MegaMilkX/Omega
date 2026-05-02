#pragma once

#include "gui/elements/label.hpp"


inline void guiLayoutSplitRect2XRatio(gfxm::rect& rc, gfxm::rect& reminder, float ratio) {
    const float w = rc.max.x - rc.min.x;
    const float w0 = w * ratio;
    const float w1 = w - w0;

    reminder = rc;
    rc.max.x = rc.min.x + w0;
    reminder.min.x = rc.max.x;
}
inline void guiLayoutSplitRect2X(gfxm::rect& rc, gfxm::rect& reminder, float left_size) {
    const float w = rc.max.x - rc.min.x;
    const float w0 = gfxm::_min(left_size, w);
    const float w1 = w - w0;

    reminder = rc;
    rc.max.x = rc.min.x + w0;
    reminder.min.x = rc.max.x;
}
inline void guiLayoutSplitRectH(const gfxm::rect& total, gfxm::rect* rects, int rect_count, float margin) {
    const float w_total = total.max.x - total.min.x;
    const float w_single = (w_total - margin * (rect_count - 1)) / rect_count;

    float start_x = total.min.x - margin;
    for (int i = 0; i < rect_count; ++i) {
        rects[i].min.x = margin + start_x + (w_single + margin) * i;
        rects[i].min.y = total.min.y;
        rects[i].max.x = rects[i].min.x + w_single;
        rects[i].max.y = total.max.y;
    }
}
