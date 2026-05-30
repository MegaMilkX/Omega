#pragma once

#include "math/gfxm.hpp"
#include "gui/gui_font.hpp"


enum GUI_MOUSE_BUTTON {
    GUI_MOUSE_LEFT,
    GUI_MOUSE_RIGHT,
    GUI_MOUSE_MID
};
enum GUI_KEY_STATE {
    GUI_KEY_DOWN,
    GUI_KEY_UP,
};

enum class GUI_HORIZONTAL_ALIGNMENT {
    LEFT,
    CENTER,
    RIGHT
};
inline float guiHorizontalAlignToFloat(GUI_HORIZONTAL_ALIGNMENT align) {
    float ret = .0f;
    switch (align) {
    case GUI_HORIZONTAL_ALIGNMENT::LEFT: break;
    case GUI_HORIZONTAL_ALIGNMENT::CENTER: ret = .5f; break;
    case GUI_HORIZONTAL_ALIGNMENT::RIGHT: ret = 1.f; break;
    default: assert(false);
    }
    return ret;
}
enum class GUI_VERTICAL_ALIGNMENT {
    TOP,
    CENTER,
    BOTTOM
};
inline float guiVerticalAlignToFloat(GUI_VERTICAL_ALIGNMENT align) {
    float ret = .0f;
    switch (align) {
    case GUI_VERTICAL_ALIGNMENT::TOP: break;
    case GUI_VERTICAL_ALIGNMENT::CENTER: ret = .5f; break;
    case GUI_VERTICAL_ALIGNMENT::BOTTOM: ret = 1.f; break;
    default: assert(false);
    }
    return ret;
}

typedef uint64_t gui_layout_flag_t;

const gui_layout_flag_t GUI_LAYOUT_NO_TITLE         = 0x00000001;
const gui_layout_flag_t GUI_LAYOUT_NO_BORDER        = 0x00000002;
const gui_layout_flag_t GUI_LAYOUT_DRAW_SHADOW      = 0x00000004;
const gui_layout_flag_t GUI_LAYOUT_FIRST_PASS       = 0x00000010; // If during the first pass we exceed the client area - the second pass occurs, taking scrollbars into account
const gui_layout_flag_t GUI_LAYOUT_WIDTH_PASS       = 0x00000020;
const gui_layout_flag_t GUI_LAYOUT_HEIGHT_PASS      = 0x00000040;
const gui_layout_flag_t GUI_LAYOUT_POSITION_PASS    = 0x00000080;
const gui_layout_flag_t GUI_LAYOUT_FIT_CONTENT      = 0x00000100;

const uint64_t GUI_SYS_FLAG_DRAG_SUBSCRIBER = 0x0001;
const uint64_t GUI_SYS_FLAG_UNUSED0         = 0x0002;

typedef uint64_t gui_flag_t;
// Stops an element from being deleted by clearChildren()
const gui_flag_t GUI_FLAG_PERSISTENT                = 0x00000001;
// Frame layout
const gui_flag_t GUI_FLAG_UNUSED_0                  = 0x00000002; // TODO: REMOVE
// Floating layout
const gui_flag_t GUI_FLAG_UNUSED_1                  = 0x00000004;
const gui_flag_t GUI_FLAG_WINDOW                    = 0x00000008; // TODO: RENAME, signifies an ability to "activate"
const gui_flag_t GUI_FLAG_TOPMOST                   = 0x00000010;
// Blocks all mouse interactions with elements behind in z-order
const gui_flag_t GUI_FLAG_BLOCKING                  = 0x00000020;
// Makes the element receive OUTSIDE_MENU message if user clicked outside it's bounds
const gui_flag_t GUI_FLAG_MENU_POPUP                = 0x00000040;
const gui_flag_t GUI_FLAG_MENU_SKIP_OWNER_CLICK     = 0x00000080;
const gui_flag_t GUI_FLAG_SAME_LINE                 = 0x00000100;
const gui_flag_t GUI_FLAG_SCROLLV                   = 0x00000200;
const gui_flag_t GUI_FLAG_SCROLLH                   = 0x00000400;
// Skips layout and draw phases for element's children as if there are none
const gui_flag_t GUI_FLAG_HIDE_CONTENT              = 0x00000800;
// Skips element's body when checking for currently hovered element
// Does not skip it's children
const gui_flag_t GUI_FLAG_NO_HIT                    = 0x00001000;
// Allows to scroll content of an element by dragging the client area with left mouse button
const gui_flag_t GUI_FLAG_UNUSED_2                  = 0x00002000;
// Skips CLICK messages if mouse moved between button down and up
const gui_flag_t GUI_FLAG_NO_PULL_CLICK             = 0x00004000;
//
const gui_flag_t GUI_FLAG_UNUSED_3                  = 0x00008000;
const gui_flag_t GUI_FLAG_SELECTED                  = 0x00010000;
const gui_flag_t GUI_FLAG_DISABLED                  = 0x00020000;

const gui_flag_t GUI_FLAG_RESIZE_X                  = 0x00040000;
const gui_flag_t GUI_FLAG_RESIZE_Y                  = 0x00080000;
const gui_flag_t GUI_FLAG_RESIZE                    = GUI_FLAG_RESIZE_X | GUI_FLAG_RESIZE_Y;

const gui_flag_t GUI_FLAG_HOVERED                   = 0x00100000;
const gui_flag_t GUI_FLAG_FOCUSABLE                 = 0x00200000;
const gui_flag_t GUI_FLAG_PRESSED                   = 0x00400000;
const gui_flag_t GUI_FLAG_FOCUSED                   = 0x00800000;

const gui_flag_t GUI_FLAG_CAPTION                   = 0x01000000;


enum gui_unit {
    gui_pixel,
    gui_line_height,
    gui_percent,
    gui_fill,
    gui_content
};

struct gui_float {
    float value = .0f;
    gui_unit unit = gui_pixel;

    gui_float() {}
    gui_float(float v) : value(v), unit(gui_pixel) {}
    gui_float(float v, gui_unit u): value(v), unit(u) {}
};

struct gui_vec2 {
    gui_float x;
    gui_float y;

    gui_vec2()
        : x(0), y(0) {}
    gui_vec2(gui_float x, gui_float y)
        : x(x), y(y) {}
    gui_vec2(float x, float y, gui_unit unit)
        : x(x, unit), y(y, unit) {}
    gui_vec2(const gfxm::vec2& sz, gui_unit unit)
        : x(sz.x, unit), y(sz.y, unit) {}
};


namespace gui {

inline gui_float px(float val) {
    return gui_float( val, gui_pixel );
}
inline gui_vec2 px(float x, float y) {
    return gui_vec2(x, y, gui_pixel);
}
inline gui_vec2 px(gfxm::vec2 val) {
    return gui_vec2(val.x, val.y, gui_pixel);
}
inline gui_float em(float val) {
    return gui_float( val, gui_line_height );
}
inline gui_vec2 em(float x, float y) {
    return gui_vec2(x, y, gui_line_height);
}
inline gui_vec2 em(gfxm::vec2 val) {
    return gui_vec2(val.x, val.y, gui_line_height);
}
inline gui_float perc(float val) {
    return gui_float( val, gui_percent );
}
inline gui_vec2 perc(float x, float y) {
    return gui_vec2(x, y, gui_percent);
}
inline gui_vec2 perc(gfxm::vec2 val) {
    return gui_vec2(val.x, val.y, gui_percent);
}

inline gui_float fill() {
    return gui_float( .0f, gui_fill );
}

inline gui_float content() {
    return gui_float( .0f, gui_content );
}


}




struct gui_rect {
    gui_vec2 min;
    gui_vec2 max;

    gui_rect() {}
    gui_rect(gui_float left, gui_float top, gui_float right, gui_float bottom)
        : min(left, top), max(right, bottom) {}
    gui_rect(gui_vec2 min, gui_vec2 max)
        : min(min), max(max) {}
};


inline gui_float gui_float_convert(gui_float v, int line_height, float _100perc_px) {
    switch (v.unit) {
    case gui_pixel:
        return v;
    case gui_line_height:
        return gui_float(v.value * line_height, gui_pixel);
    case gui_percent:
        return gui_float(v.value * 0.01f * _100perc_px, gui_pixel);
    case gui_fill:
        return gui_float(.0f, gui_fill);
    case gui_content:
        return gui_float(.0f, gui_content);
    }
    assert(false);
    return gui_float(.0f, gui_pixel);
}
inline gui_vec2 gui_float_convert(gui_vec2 v2, int line_height, gfxm::vec2 _100perc_px) {
    return gui_vec2(
        gui_float_convert(v2.x, line_height, _100perc_px.x),
        gui_float_convert(v2.y, line_height, _100perc_px.y)
    );
}
inline gui_rect gui_float_convert(gui_rect rc, int line_height, gfxm::vec2 _100perc_px) {
    return gui_rect(
        gui_float_convert(rc.min.x, line_height, _100perc_px.x),
        gui_float_convert(rc.min.y, line_height, _100perc_px.y),
        gui_float_convert(rc.max.x, line_height, _100perc_px.x),
        gui_float_convert(rc.max.y, line_height, _100perc_px.y)
    );
}

inline gui_float gui_float_convert(gui_float v, Font* font, float _100perc_px) {
    return gui_float_convert(v, font->getLineHeight(), _100perc_px);
}
inline gui_vec2 gui_float_convert(gui_vec2 v2, Font* font, gfxm::vec2 _100perc_px) {
    return gui_float_convert(v2, font->getLineHeight(), _100perc_px);
}
inline gui_rect gui_float_convert(gui_rect rc, Font* font, gfxm::vec2 _100perc_px) {
    return gui_float_convert(rc, font->getLineHeight(), _100perc_px);
}

inline float gui_to_px(gui_float v, int line_height, float _100perc_px) {
    switch (v.unit) {
    case gui_pixel:
        return v.value;
    case gui_line_height:
        return v.value * line_height;
    case gui_percent:
        return v.value * 0.01f * _100perc_px;
    case gui_fill:
        return _100perc_px;
    case gui_content:
        return .0f;
    }
    assert(false);
    return .0f;
}
inline gfxm::vec2 gui_to_px(gui_vec2 v, int line_height, gfxm::vec2 _100perc_px) {
    return gfxm::vec2(
        gui_to_px(v.x, line_height, _100perc_px.x),
        gui_to_px(v.y, line_height, _100perc_px.y)
    );
}
inline gfxm::rect gui_to_px(gui_rect rc, int line_height, gfxm::vec2 _100perc_size_px) {
    return gfxm::rect(
        gui_to_px(rc.min.x, line_height, _100perc_size_px.x),
        gui_to_px(rc.min.y, line_height, _100perc_size_px.y),
        gui_to_px(rc.max.x, line_height, _100perc_size_px.x),
        gui_to_px(rc.max.y, line_height, _100perc_size_px.y)
    );
}

inline float gui_to_px(gui_float v, Font* font, float _100perc_px) {
    return gui_to_px(v, font->getLineHeight(), _100perc_px);
}
inline gfxm::vec2 gui_to_px(gui_vec2 v2, Font* font, gfxm::vec2 _100perc_px) {
    return gui_to_px(v2, font->getLineHeight(), _100perc_px);
}
inline gfxm::rect gui_to_px(gui_rect rc, Font* font, gfxm::vec2 _100perc_px) {
    return gui_to_px(rc, font->getLineHeight(), _100perc_px);
}

