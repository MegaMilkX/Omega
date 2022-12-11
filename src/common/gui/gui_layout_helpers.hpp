#pragma once

#include "math/gfxm.hpp"

typedef int GUI_ALIGNMENT;

constexpr GUI_ALIGNMENT GUI_LEFT      = 0x000001;
constexpr GUI_ALIGNMENT GUI_RIGHT     = 0x000002;
constexpr GUI_ALIGNMENT GUI_TOP       = 0x000004;
constexpr GUI_ALIGNMENT GUI_BOTTOM    = 0x000008;
constexpr GUI_ALIGNMENT GUI_HCENTER   = GUI_LEFT | GUI_RIGHT;
constexpr GUI_ALIGNMENT GUI_VCENTER   = GUI_TOP | GUI_BOTTOM;


inline gfxm::rect guiLayoutPlaceRectInsideRect(
    const gfxm::rect& container,
    const gfxm::vec2& contained_sz,
    GUI_ALIGNMENT alignment,
    const gfxm::rect& padding = gfxm::rect(0, 0, 0, 0)
) {
    gfxm::rect padded = container;
    padded.min.x += padding.min.x;
    padded.min.y += padding.min.y;
    padded.max.x -= padding.max.x;
    padded.max.y -= padding.max.y;
    gfxm::vec2 container_sz(padded.max.x - padded.min.x, padded.max.y - padded.min.y);
    gfxm::vec2 ret;
    if ((alignment & GUI_HCENTER) == GUI_HCENTER) {
        ret.x = padded.min.x + container_sz.x * .5f - contained_sz.x * .5f;
    } else if((alignment & GUI_LEFT) == GUI_LEFT) {
        ret.x = padded.min.x;
    } else if((alignment & GUI_RIGHT) == GUI_RIGHT) {
        ret.x = padded.max.x - contained_sz.x;
    }
    if ((alignment & GUI_VCENTER) == GUI_VCENTER) {
        ret.y = padded.min.y + container_sz.y * .5f - contained_sz.y * .5f;
    } else if((alignment & GUI_TOP) == GUI_TOP) {
        ret.y = padded.min.y;
    } else if((alignment & GUI_BOTTOM) == GUI_TOP) {
        ret.y = padded.max.y - contained_sz.y;
    }
    return gfxm::rect(ret, ret + contained_sz);
}

inline gfxm::rect guiLayoutPlaceRectInsideRect(
    const gfxm::rect& container,
    const gfxm::rect& contained,
    GUI_ALIGNMENT alignment,
    const gfxm::rect& padding = gfxm::rect(0, 0, 0, 0)
) {
    return guiLayoutPlaceRectInsideRect(
        container, contained.max - contained.min, alignment, padding
    );
}