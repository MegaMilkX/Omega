#pragma once

#include "math/gfxm.hpp"
#include "gui/gui_font.hpp"

enum gui_unit {
    gui_pixel,
    gui_line_height,
    gui_percent
};

struct gui_float {
    float value;
    gui_unit unit;

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
};


namespace gui {

inline gui_float px(float val) {
    return gui_float( val, gui_pixel );
}
inline gui_vec2 px(gfxm::vec2 val) {
    return gui_vec2(val.x, val.y, gui_pixel);
}
inline gui_float em(float val) {
    return gui_float( val, gui_line_height );
}
inline gui_vec2 em(gfxm::vec2 val) {
    return gui_vec2(val.x, val.y, gui_line_height);
}
inline gui_float perc(float val) {
    return gui_float( val, gui_percent );
}
inline gui_vec2 perc(gfxm::vec2 val) {
    return gui_vec2(val.x, val.y, gui_percent);
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


inline float gui_to_px(gui_float v, GuiFont* font, float _100perc_px) {
    switch (v.unit) {
    case gui_pixel:
        return v.value;
    case gui_line_height:
        return v.value * font->font->getLineHeight();
    case gui_percent:
        return v.value * 0.01f * _100perc_px;
    }
    assert(false);
    return .0f;
}
inline gfxm::vec2 gui_to_px(gui_vec2 v, GuiFont* font, gfxm::vec2 _100perc_px) {
    return gfxm::vec2(gui_to_px(v.x, font, _100perc_px.x), gui_to_px(v.y, font, _100perc_px.y));
}

