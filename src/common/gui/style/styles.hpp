#pragma once

#include "style_component.hpp"
#include "math/gfxm.hpp"
#include "typeface/font.hpp"


namespace gui {

    constexpr int TMP_DEFAULT_DPI = 72;
    
    struct style_font : public style_component_t<style_font, true> {
        style_font() {}
        style_font(const char* font_fpath, int height) {}

        void on_merge(const style_font& other, bool empty_inherits) override {
            font_fpath.merge(other.font_fpath, empty_inherits);
            height.merge(other.height, empty_inherits);
        }
        void on_inherit(const style_font& other) override {
            font_fpath.inherit(other.font_fpath);
            height.inherit(other.height);
        }

        void finalize() override {
            if (!font_fpath.has_value() || !height.has_value()) {
                font.reset();
                return;
            }
            if (font_fpath.value().empty()) {
                font.reset();
                return;
            }
            font = fontGet(font_fpath.value().c_str(), height.value(), TMP_DEFAULT_DPI);
        }

        style_value<std::string> font_fpath = inherit;
        style_value<int>         height = inherit;

        std::shared_ptr<Font> font;
    };

    struct style_box : public style_component_t<style_box, false> {
        style_box() {}
        style_box(
            style_value<gui_float> width,
            style_value<gui_float> height,
            style_value<gui_rect> margin,
            style_value<gui_vec2> content_margin,
            style_value<gui_rect> padding
        ) : width(width),
            height(height),
            margin(margin),
            content_margin(content_margin),
            padding(padding)
        {}

        void on_merge(const style_box& other, bool empty_inherits) override {
            width.merge(other.width, empty_inherits);
            height.merge(other.height, empty_inherits);
            margin.merge(other.margin, empty_inherits);
            content_margin.merge(other.content_margin, empty_inherits);
            padding.merge(other.padding, empty_inherits);
        }
        void on_inherit(const style_box& other) override {
            width.inherit(other.width);
            height.inherit(other.height);
            margin.inherit(other.margin);
            content_margin.inherit(other.content_margin);
            padding.inherit(other.padding);
        }

        style_value<gui_float> width;
        style_value<gui_float> height;
        style_value<gui_rect> margin;
        style_value<gui_vec2> content_margin;
        style_value<gui_rect> padding;
    };

    struct style_color : public style_component_t<style_color, false> {
        style_color() {}
        style_color(uint32_t color)
            : color(color) {}

        void on_merge(const style_color& other, bool empty_inherits) override {
            color.merge(other.color, empty_inherits);
        }
        void on_inherit(const style_color& other) override {
            color.inherit(other.color);
        }

        style_value<uint32_t> color;
    };

    struct style_background_color : public style_component_t<style_background_color, false> {
        style_background_color() {}
        style_background_color(uint32_t color)
            : color(color) {}

        void on_merge(const style_background_color& other, bool empty_inherits) override {
            color.merge(other.color, empty_inherits);
        }
        void on_inherit(const style_background_color& other) override {
            color.inherit(other.color);
        }

        style_value<uint32_t> color;
    };

    struct style_border_radius : public style_component_t<style_border_radius, false> {
        style_border_radius() {}
        style_border_radius(gui_float radius_nw, gui_float radius_ne, gui_float radius_se, gui_float radius_sw)
            : radius_top_left(radius_nw), radius_top_right(radius_ne), radius_bottom_right(radius_se), radius_bottom_left(radius_sw) {}

        void on_merge(const style_border_radius& other, bool empty_inherits) override {
            radius_top_left.merge(other.radius_top_left, empty_inherits);
            radius_top_right.merge(other.radius_top_right, empty_inherits);
            radius_bottom_right.merge(other.radius_bottom_right, empty_inherits);
            radius_bottom_left.merge(other.radius_bottom_left, empty_inherits);
        }
        void on_inherit(const style_border_radius& other) override {
            radius_top_left.inherit(other.radius_top_left);
            radius_top_right.inherit(other.radius_top_right);
            radius_bottom_right.inherit(other.radius_bottom_right);
            radius_bottom_left.inherit(other.radius_bottom_left);
        }

        style_value<gui_float> radius_top_left;
        style_value<gui_float> radius_top_right;
        style_value<gui_float> radius_bottom_right;
        style_value<gui_float> radius_bottom_left;
    };

    struct style_border : public style_component_t<style_border, false> {
        style_border() {}
        style_border(
            gui_float thickness_left, gui_float thickness_top, gui_float thickness_right, gui_float thickness_bottom,
            uint32_t color_left, uint32_t color_top, uint32_t color_right, uint32_t color_bottom
        ) : thickness_left(thickness_left), thickness_top(thickness_top), thickness_right(thickness_right), thickness_bottom(thickness_bottom),
            color_left(color_left), color_top(color_top), color_right(color_right), color_bottom(color_bottom) {}

        void on_merge(const style_border& other, bool empty_inherits) override {
            thickness_left.merge(other.thickness_left, empty_inherits);
            thickness_top.merge(other.thickness_top, empty_inherits);
            thickness_right.merge(other.thickness_right, empty_inherits);
            thickness_bottom.merge(other.thickness_bottom, empty_inherits);
            color_left.merge(other.color_left, empty_inherits);
            color_top.merge(other.color_top, empty_inherits);
            color_right.merge(other.color_right, empty_inherits);
            color_bottom.merge(other.color_bottom, empty_inherits);
        }
        void on_inherit(const style_border& other) override {
            thickness_left.inherit(other.thickness_left);
            thickness_top.inherit(other.thickness_top);
            thickness_right.inherit(other.thickness_right);
            thickness_bottom.inherit(other.thickness_bottom);
            color_left.inherit(other.color_left);
            color_top.inherit(other.color_top);
            color_right.inherit(other.color_right);
            color_bottom.inherit(other.color_bottom);
        }

        style_value<gui_float> thickness_left;
        style_value<gui_float> thickness_top;
        style_value<gui_float> thickness_right;
        style_value<gui_float> thickness_bottom;
        style_value<uint32_t> color_left;
        style_value<uint32_t> color_top;
        style_value<uint32_t> color_right;
        style_value<uint32_t> color_bottom;
    };

}


namespace gui {

    template<typename COMPONENT_T, typename MEMBER_T, typename VALUE_T>
    void apply_style_prop(style& s, MEMBER_T COMPONENT_T::*member_ptr, const VALUE_T& value) {
        auto component = s.get_component<COMPONENT_T>();
        if (!component) {
            component = s.add_component<COMPONENT_T>();
        }
        component->*member_ptr = value;
    }

    struct font_file : public style_prop {
        std::string path;
        font_file(const std::string& path) : path(path) {}
        void apply(style& s) const override {
            apply_style_prop(s, &style_font::font_fpath, path);
        }
        style_prop* clone_new() const override {
            return new font_file(*this);
        }
    };
    struct font_size : public style_prop {
        int size;
        font_size(int size) : size(size) {}
        void apply(style& s) const override {
            apply_style_prop(s, &style_font::height, size);
        }
        style_prop* clone_new() const override {
            return new font_size(*this);
        }
    };
    struct color : public style_prop {
        uint32_t c;
        color(uint32_t c) : c(c) {}
        void apply(style& s) const override {
            apply_style_prop(s, &style_color::color, c);
        }
        style_prop* clone_new() const override {
            return new color(*this);
        }
    };
    struct background_color : public style_prop {
        uint32_t c;
        background_color(uint32_t c) : c(c) {}
        void apply(style& s) const override {
            apply_style_prop(s, &style_background_color::color, c);
        }
        style_prop* clone_new() const override {
            return new background_color(*this);
        }
    };
    struct margin : public style_prop {
        gui_rect rc;
        margin(gui_float ltrb) : rc(ltrb, ltrb, ltrb, ltrb) {}
        margin(gui_float h, gui_float v) : rc(h, v, h, v) {}
        margin(gui_float left, gui_float top, gui_float right, gui_float bottom) : rc(left, top, right, bottom) {}
        margin(gui_rect rc) : rc(rc) {}
        void apply(style& s) const override {
            apply_style_prop(s, &style_box::margin, rc);
        }
        style_prop* clone_new() const override {
            return new margin(*this);
        }
    };
    struct content_margin : public style_prop {
        gui_vec2 extents;
        content_margin(gui_float x, gui_float y) : extents(x, y) {}
        content_margin(gui_float m) : extents(m, m) {}
        void apply(style& s) const override {
            apply_style_prop(s, &style_box::content_margin, extents);
        }
        style_prop* clone_new() const override {
            return new content_margin(*this);
        }
    };
    struct padding : public style_prop {
        gui_rect rc;
        padding(gui_float left, gui_float top, gui_float right, gui_float bottom) : rc(left, top, right, bottom) {}
        padding(gui_rect rc) : rc(rc) {}
        void apply(style& s) const override {
            apply_style_prop(s, &style_box::padding, rc);
        }
        style_prop* clone_new() const override {
            return new padding(*this);
        }
    };
    struct border_radius : public style_prop {
        gui_rect rc;
        border_radius(gui_float left, gui_float top, gui_float right, gui_float bottom) : rc(left, top, right, bottom) {}
        border_radius(gui_rect rc) : rc(rc) {}
        void apply(style& s) const override {
            apply_style_prop(s, &style_border_radius::radius_top_left, rc.min.x);
            apply_style_prop(s, &style_border_radius::radius_top_right, rc.min.y);
            apply_style_prop(s, &style_border_radius::radius_bottom_right, rc.max.x);
            apply_style_prop(s, &style_border_radius::radius_bottom_left, rc.max.y);
        }
        style_prop* clone_new() const override {
            return new border_radius(*this);
        }
    };
    struct border_thickness : public style_prop {
        gui_rect rc;
        border_thickness(gui_float left, gui_float top, gui_float right, gui_float bottom) : rc(left, top, right, bottom) {}
        border_thickness(gui_rect rc) : rc(rc) {}
        void apply(style& s) const override {
            apply_style_prop(s, &style_border::thickness_left, rc.min.x);
            apply_style_prop(s, &style_border::thickness_top, rc.min.y);
            apply_style_prop(s, &style_border::thickness_right, rc.max.x);
            apply_style_prop(s, &style_border::thickness_bottom, rc.max.y);
        }
        style_prop* clone_new() const override {
            return new border_thickness(*this);
        }
    };
    struct border_color : public style_prop {
        uint32_t left;
        uint32_t top;
        uint32_t right;
        uint32_t bottom;
        border_color(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) : left(left), top(top), right(right), bottom(bottom) {}
        void apply(style& s) const override {
            apply_style_prop(s, &style_border::color_left, left);
            apply_style_prop(s, &style_border::color_top, top);
            apply_style_prop(s, &style_border::color_right, right);
            apply_style_prop(s, &style_border::color_bottom, bottom);
        }
        style_prop* clone_new() const override {
            return new border_color(*this);
        }
    };
}
