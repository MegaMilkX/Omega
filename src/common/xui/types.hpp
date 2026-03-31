#pragma once

#include "math/gfxm.hpp"


namespace xui {


    enum unit_t {
        u_pixel,
        u_line_height,
        u_percent,
        u_fill,
        u_content
    };

    struct xfloat {
        float value = .0f;
        unit_t unit = u_pixel;

        xfloat() {}
        xfloat(float v) : value(v), unit(u_pixel) {}
        xfloat(float v, unit_t u): value(v), unit(u) {}
    };

    struct xvec2 {
        xfloat x;
        xfloat y;

        xvec2()
            : x(0), y(0) {}
        xvec2(xfloat x, xfloat y)
            : x(x), y(y) {}
        xvec2(float x, float y, unit_t unit)
            : x(x, unit), y(y, unit) {}
        xvec2(const gfxm::vec2& sz, unit_t unit)
            : x(sz.x, unit), y(sz.y, unit) {}
    };
    
    struct xrect {
        xvec2 min;
        xvec2 max;

        xrect() {}
        xrect(xfloat left, xfloat top, xfloat right, xfloat bottom)
            : min(left, top), max(right, bottom) {}
        xrect(xvec2 min, xvec2 max)
            : min(min), max(max) {}
    };

    inline xfloat px(float val) {
        return xfloat( val, u_pixel );
    }
    inline xvec2 px(float x, float y) {
        return xvec2(x, y, u_pixel);
    }
    inline xfloat em(float val) {
        return xfloat( val, u_line_height );
    }
    inline xvec2 em(float x, float y) {
        return xvec2(x, y, u_line_height);
    }
    inline xfloat perc(float val) {
        return xfloat( val, u_percent );
    }
    inline xvec2 perc(float x, float y) {
        return xvec2(x, y, u_percent);
    }
    inline xfloat fill() {
        return xfloat( .0f, u_fill );
    }
    inline xfloat content() {
        return xfloat( .0f, u_content );
    }

    inline xfloat float_convert(xfloat v, float line_height, float _100perc_px) {
        switch (v.unit) {
        case u_pixel:
            return v;
        case u_line_height:
            return xfloat(v.value * line_height, u_pixel);
        case u_percent:
            return xfloat(v.value * 0.01f * _100perc_px, u_pixel);
        case u_fill:
            return xfloat(.0f, u_fill);
        case u_content:
            return xfloat(.0f, u_content);
        }
        assert(false);
        return xfloat(.0f, u_pixel);
    }
    inline xvec2 float_convert(xvec2 v2, float line_height, gfxm::vec2 _100perc_px) {
        return xvec2(
            float_convert(v2.x, line_height, _100perc_px.x),
            float_convert(v2.y, line_height, _100perc_px.y)
        );
    }
    inline xrect float_convert(xrect rc, float line_height, gfxm::vec2 _100perc_px) {
        return xrect(
            float_convert(rc.min.x, line_height, _100perc_px.x),
            float_convert(rc.min.y, line_height, _100perc_px.y),
            float_convert(rc.max.x, line_height, _100perc_px.x),
            float_convert(rc.max.y, line_height, _100perc_px.y)
        );
    }

    inline float to_px(xfloat v, float line_height, float _100perc_px) {
        switch (v.unit) {
        case u_pixel:
            return v.value;
        case u_line_height:
            return v.value * line_height;
        case u_percent:
            return v.value * 0.01f * _100perc_px;
        case u_fill:
            return _100perc_px;
        case u_content:
            return .0f;
        }
        assert(false);
        return .0f;
    }
    inline gfxm::vec2 to_px(xvec2 v, float line_height, gfxm::vec2 _100perc_px) {
        return gfxm::vec2(
            to_px(v.x, line_height, _100perc_px.x),
            to_px(v.y, line_height, _100perc_px.y)
        );
    }
    inline gfxm::rect to_px(xrect rc, float line_height, gfxm::vec2 _100perc_size_px) {
        return gfxm::rect(
            to_px(rc.min.x, line_height, _100perc_size_px.x),
            to_px(rc.min.y, line_height, _100perc_size_px.y),
            to_px(rc.max.x, line_height, _100perc_size_px.x),
            to_px(rc.max.y, line_height, _100perc_size_px.y)
        );
    }

}

