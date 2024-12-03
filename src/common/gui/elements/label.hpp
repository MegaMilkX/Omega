#pragma once

#include "gui/elements/element.hpp"


class GuiLabel : public GuiElement {
    GuiTextBuffer text_caption;
    gfxm::vec2 pos_caption;
public:
    GuiLabel(const char* caption = "Label") {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        setStyleClasses({ "control" });
        Font* font = getFont();
        text_caption.replaceAll(font, caption, strlen(caption));
        text_caption.prepareDraw(font, false);
        size.x = text_caption.getBoundingSize().x + GUI_PADDING * 2.f;
        size.y = font->getLineHeight() * 2.f;
    }

    void setCaption(const char* cap) {
        text_caption.replaceAll(getFont(), cap, strlen(cap));
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        return;
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        Font* font = getFont();
        gfxm::vec2 px_size = gui_to_px(size, font, gfxm::vec2(rc.max.x - rc.min.x, rc.max.y - rc.min.y));
        rc_bounds = gfxm::rect(
            rc.min,
            rc.min + px_size
        );
        client_area = rc_bounds;
        pos_caption = rc_bounds.min;
        pos_caption.x += GUI_PADDING;
        const float content_y_offset = px_size.y * .5f - (font->getAscender() + font->getDescender()) * .5f;
        pos_caption.y += content_y_offset;

        text_caption.prepareDraw(font, false);
        gfxm::vec2 text_size = text_caption.getBoundingSize();
        client_area.max.x = client_area.min.x + text_size.x + GUI_PADDING * 2.f;
        client_area.max.y = client_area.min.y + text_size.y + content_y_offset * 2.f;
        rc_bounds = client_area;
    }
    void onDraw() override {
        {
            Font* font = getFont();

            GuiElement* e = this;
            gfxm::rect rc = e->getBoundingRect();
            float rc_width = rc.max.x - rc.min.x;
            float rc_height = rc.max.y - rc.min.y;
            float min_side = gfxm::_min(rc_width, rc_height);
            float half_min_side = min_side * .5f;
            auto bg_color_style = e->getStyleComponent<gui::style_background_color>();
            auto border_style = e->getStyleComponent<gui::style_border>();
            auto border_radius_style = e->getStyleComponent<gui::style_border_radius>();

            if (bg_color_style) {
                if (border_radius_style) {
                    float br_tl = gui_to_px(border_radius_style->radius_top_left.value(), font, half_min_side);
                    float br_tr = gui_to_px(border_radius_style->radius_top_right.value(), font, half_min_side);
                    float br_bl = gui_to_px(border_radius_style->radius_bottom_left.value(), font, half_min_side);
                    float br_br = gui_to_px(border_radius_style->radius_bottom_right.value(), font, half_min_side);
                    guiDrawRectRound(
                        rc, br_tl, br_tr, br_bl, br_br,
                        bg_color_style->color.value()
                    );
                } else {
                    guiDrawRect(rc, bg_color_style->color.value());
                }
            }
            if (border_style) {
                if (border_radius_style) {
                    float br_tl = gui_to_px(border_radius_style->radius_top_left.value(), font, half_min_side);
                    float br_tr = gui_to_px(border_radius_style->radius_top_right.value(), font, half_min_side);
                    float br_bl = gui_to_px(border_radius_style->radius_bottom_left.value(), font, half_min_side);
                    float br_br = gui_to_px(border_radius_style->radius_bottom_right.value(), font, half_min_side);
                    float bt_l = gui_to_px(border_style->thickness_left.value(), font, rc_width * .5f);
                    float bt_t = gui_to_px(border_style->thickness_top.value(), font, rc_height * .5f);
                    float bt_r = gui_to_px(border_style->thickness_right.value(), font, rc_width * .5f);
                    float bt_b = gui_to_px(border_style->thickness_bottom.value(), font, rc_height * .5f);
                    guiDrawRectRoundBorder(
                        rc, br_tl, br_tr, br_bl, br_br,
                        bt_l, bt_t, bt_r, bt_b,
                        border_style->color_left.value(), border_style->color_top.value(), border_style->color_right.value(), border_style->color_bottom.value()
                    );
                } else {
                    // TODO:
                }
            }
        }

        text_caption.draw(getFont(), pos_caption, GUI_COL_TEXT, GUI_COL_ACCENT);
    }
};