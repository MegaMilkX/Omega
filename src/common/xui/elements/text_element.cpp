#include "text_element.hpp"
#include "xui/host.hpp"
#include "gui/style/styles.hpp"


namespace xui {


    TextElement::TextElement(const std::string& str) {
        size = gui_vec2(gui::content(), gui::content());
        string = str;
    }

    static TextLayout::HALIGN guiConvertHorizontalAlignmentToTextLayout(GUI_HORIZONTAL_ALIGNMENT halign) {
        switch (halign) {
        case GUI_HORIZONTAL_ALIGNMENT::LEFT: return TextLayout::HALIGN_LEFT;
        case GUI_HORIZONTAL_ALIGNMENT::CENTER: return TextLayout::HALIGN_CENTER;
        case GUI_HORIZONTAL_ALIGNMENT::RIGHT: return TextLayout::HALIGN_RIGHT;
        default:
            assert(false);
            return TextLayout::HALIGN_LEFT;
        }
    }
    static TextLayout::VALIGN guiConvertVerticalAlignmentToTextLayout(GUI_VERTICAL_ALIGNMENT halign) {
        switch (halign) {
        case GUI_VERTICAL_ALIGNMENT::TOP: return TextLayout::VALIGN_TOP;
        case GUI_VERTICAL_ALIGNMENT::CENTER: return TextLayout::VALIGN_CENTER;
        case GUI_VERTICAL_ALIGNMENT::BOTTOM: return TextLayout::VALIGN_BOTTOM;
        default:
            assert(false);
            return TextLayout::VALIGN_TOP;
        }
    }

    void TextElement::layout(Host* host, LayoutContext& ctx) {
        for (LAYOUT_PHASE ph = next_layout_phase; ph <= ctx.phase; ++ph) {
            switch (ph) {
            case LAYOUT_PHASE::WIDTH: {
                cached_font = host->resolveFont(this);

                uint32_t color = 0xFFFFFFFF;
                auto style_color = style->get_component<gui::style_color>();
                if (style_color) {
                    color = style_color->color.value(0xFFFFFFFF);
                }
                text_layout.base_color = color;

                GUI_HORIZONTAL_ALIGNMENT halign = GUI_HORIZONTAL_ALIGNMENT::LEFT;
                gui_rect padding;
                auto style = getStyle();
                auto style_box = style->get_component<gui::style_box>();
                if (style_box) {
                    halign = style_box->horizontal_align.value(GUI_HORIZONTAL_ALIGNMENT::LEFT);
                    padding = style_box->padding.value(gui_rect(gui::px(0), gui::px(0), gui::px(0), gui::px(0)));                    
                }
                TextLayout::HALIGN tl_halign = guiConvertHorizontalAlignmentToTextLayout(halign);

                gui_rect border_thickness;
                auto style_border = style->get_component<gui::style_border>();
                if (style_border) {
                    border_thickness = gui_rect(
                        style_border->thickness_left.value(0),
                        style_border->thickness_top.value(0),
                        style_border->thickness_right.value(0),
                        style_border->thickness_bottom.value(0)
                    );
                }

                if(ctx.px_resolved_width.has_value()) {
                    gfxm::rect px_padding = gui_to_px(padding, resolved_font->getLineHeight(), gfxm::vec2(ctx.px_resolved_width.value(), 0));
                    gfxm::rect px_border = gui_to_px(border_thickness, resolved_font->getLineHeight(), gfxm::vec2(ctx.px_resolved_width.value(), 0));

                    int box_width = ctx.px_resolved_width.value();
                    box_width = gfxm::_max(.0f, box_width - (px_padding.min.x + px_padding.max.x));

                    text_layout.build(string, cached_font, box_width);
                    text_layout.alignHorizontal(tl_halign, box_width);

                    text_layout.padHorizontal(px_padding.min.x + px_border.min.x, px_padding.max.x + px_border.max.x);
                } else {
                    text_layout.build(string, cached_font, -1);
                    text_layout.alignHorizontal(tl_halign, -1);

                    gfxm::rect px_padding = gui_to_px(padding, resolved_font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));
                    gfxm::rect px_border = gui_to_px(border_thickness, resolved_font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));
                    text_layout.padHorizontal(px_padding.min.x + px_border.min.x, px_padding.max.x + px_border.max.x);
                }
                ctx.px_measured_width = text_layout.bounding_width;
                next_layout_phase = LAYOUT_PHASE::HEIGHT;
                break;
            }
            case LAYOUT_PHASE::HEIGHT: {
                GUI_VERTICAL_ALIGNMENT valign = GUI_VERTICAL_ALIGNMENT::TOP;
                gui_rect padding;
                auto style = getStyle();
                auto style_box = style->get_component<gui::style_box>();
                if (style_box) {
                    valign = style_box->vertical_align.value(GUI_VERTICAL_ALIGNMENT::TOP);
                    padding = style_box->padding.value(gui_rect(gui::px(0), gui::px(0), gui::px(0), gui::px(0)));
                }
                TextLayout::VALIGN tl_valign = guiConvertVerticalAlignmentToTextLayout(valign);

                gui_rect border_thickness;
                auto style_border = style->get_component<gui::style_border>();
                if (style_border) {
                    border_thickness = gui_rect(
                        style_border->thickness_left.value(0),
                        style_border->thickness_top.value(0),
                        style_border->thickness_right.value(0),
                        style_border->thickness_bottom.value(0)
                    );
                }

                if (ctx.px_resolved_height.has_value()) {
                    text_layout.alignVertical(tl_valign, ctx.px_resolved_height.value());
                }

                gfxm::rect px_padding = gui_to_px(padding, resolved_font->getLineHeight(), gfxm::vec2(0, text_layout.bounding_height));
                gfxm::rect px_border = gui_to_px(border_thickness, resolved_font->getLineHeight(), gfxm::vec2(0, text_layout.bounding_height));
                text_layout.padVertical(px_padding.min.y + px_border.min.y, px_padding.max.y + px_border.max.y);

                ctx.px_measured_height = text_layout.bounding_height;
                next_layout_phase = LAYOUT_PHASE::COMMIT;
                break;
            }
            case LAYOUT_PHASE::COMMIT:
                px_size = gfxm::ivec2(ctx.px_resolved_width.value(), ctx.px_resolved_height.value());
                next_layout_phase = LAYOUT_PHASE::WIDTH;
                break;
            }
        }
    }

    void TextElement::onDraw(IRenderer* r) {
        Element::onDraw(r);

        std::vector<TextVertex> vertices;
        for (int i = 0; i < text_layout.glyphs.size(); ++i) {
            const auto& g = text_layout.glyphs[i];
            const auto& q = g.makeQuad();

            uint32_t color = g.color;

            // shadow
            const gfxm::vec3 shadow_offs = gfxm::vec3(1.f, 1.f, .0f);
            vertices.insert(
                vertices.end(),
                {
                    TextVertex{ q.pos[0] + shadow_offs, q.uv[0], 0xFF000000, q.lut_values[0] },
                    TextVertex{ q.pos[1] + shadow_offs, q.uv[1], 0xFF000000, q.lut_values[1] },
                    TextVertex{ q.pos[2] + shadow_offs, q.uv[2], 0xFF000000, q.lut_values[2] },
                    TextVertex{ q.pos[1] + shadow_offs, q.uv[1], 0xFF000000, q.lut_values[1] },
                    TextVertex{ q.pos[3] + shadow_offs, q.uv[3], 0xFF000000, q.lut_values[3] },
                    TextVertex{ q.pos[2] + shadow_offs, q.uv[2], 0xFF000000, q.lut_values[2] }
                }
            );
            // text
            vertices.insert(
                vertices.end(),
                {
                    TextVertex{ q.pos[0], q.uv[0], color, q.lut_values[0] },
                    TextVertex{ q.pos[1], q.uv[1], color, q.lut_values[1] },
                    TextVertex{ q.pos[2], q.uv[2], color, q.lut_values[2] },
                    TextVertex{ q.pos[1], q.uv[1], color, q.lut_values[1] },
                    TextVertex{ q.pos[3], q.uv[3], color, q.lut_values[3] },
                    TextVertex{ q.pos[2], q.uv[2], color, q.lut_values[2] }
                }
            );
        }

        r->drawText(vertices.data(), vertices.size(), cached_font);
    }

    void TextElement::setText(const std::string& str) {
        string = str;
    }


}