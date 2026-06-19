#include "text_layout.hpp"
#include "gui/elements/text_element.hpp"

static TextLayout::HALIGN convertHorizontalAlignmentToTextLayout(GUI_HORIZONTAL_ALIGNMENT halign) {
    switch (halign) {
    case GUI_HORIZONTAL_ALIGNMENT::LEFT: return TextLayout::HALIGN_LEFT;
    case GUI_HORIZONTAL_ALIGNMENT::CENTER: return TextLayout::HALIGN_CENTER;
    case GUI_HORIZONTAL_ALIGNMENT::RIGHT: return TextLayout::HALIGN_RIGHT;
    default:
        assert(false);
        return TextLayout::HALIGN_LEFT;
    }
}
static TextLayout::VALIGN convertVerticalAlignmentToTextLayout(GUI_VERTICAL_ALIGNMENT halign) {
    switch (halign) {
    case GUI_VERTICAL_ALIGNMENT::TOP: return TextLayout::VALIGN_TOP;
    case GUI_VERTICAL_ALIGNMENT::CENTER: return TextLayout::VALIGN_CENTER;
    case GUI_VERTICAL_ALIGNMENT::BOTTOM: return TextLayout::VALIGN_BOTTOM;
    default:
        assert(false);
        return TextLayout::VALIGN_TOP;
    }
}

void GuiTextLayout::onFontChanged(GuiTextElement* elem, Font* font) {
    elem->text_layout.setFont(font);
}
int GuiTextLayout::onMeasureWidth(GuiTextElement* elem, const std::optional<int>& height_constraint) {
    width_constrained = false;

    Font* font = elem->getFont();
    auto style = elem->getStyle();

    auto& string_utf8 = elem->string_utf8;
    auto& text_layout = elem->text_layout;

    gui_rect padding;
    auto style_box = style->get_component<gui::style_box>();
    if (style_box) {
        padding = style_box->padding.value(gui_rect(gui::px(0), gui::px(0), gui::px(0), gui::px(0)));                    
    }
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
    gfxm::rect px_padding = gui_to_px(padding, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));
    gfxm::rect px_border = gui_to_px(border_thickness, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));

    text_layout.setPadding(px_padding.min.x, px_padding.max.x, px_padding.min.y, px_padding.max.y);
    text_layout.setWidth(std::nullopt);
    text_layout.build(TextLayout::BuildMode::TellWidth);

    return text_layout.bounding_width;
}

int GuiTextLayout::onMeasureHeight(GuiTextElement* elem, const std::optional<int>& width_constraint) {
    height_constrained = false;

    Font* font = elem->getFont();
    auto style = elem->getStyle();

    auto& string_utf8 = elem->string_utf8;
    auto& text_layout = elem->text_layout;

    gui_rect padding;
    auto style_box = style->get_component<gui::style_box>();
    if (style_box) {
        padding = style_box->padding.value(gui_rect(gui::px(0), gui::px(0), gui::px(0), gui::px(0)));                    
    }
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
    gfxm::rect px_padding = gui_to_px(padding, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));
    gfxm::rect px_border = gui_to_px(border_thickness, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));

    text_layout.setPadding(px_padding.min.x, px_padding.max.x, px_padding.min.y, px_padding.max.y);
    text_layout.setWidth(width_constrained ? width_constraint : std::nullopt);
    text_layout.build(TextLayout::BuildMode::TellHeight);

    return text_layout.bounding_height;
}

void GuiTextLayout::onLayout(GuiTextElement* elem, const gui_layout_context& ctx) {
    Font* font = elem->getFont();
    auto style = elem->getStyle();

    auto& string_utf8 = elem->string_utf8;
    auto& text_layout = elem->text_layout;
    
    uint32_t color = 0xFFFFFFFF;
    auto style_color = style->get_component<gui::style_color>();
    if (style_color) {
        color = style_color->color.value(0xFFFFFFFF);
    }
    text_layout.base_color = color;

    GUI_HORIZONTAL_ALIGNMENT halign = GUI_HORIZONTAL_ALIGNMENT::LEFT;
    GUI_VERTICAL_ALIGNMENT valign = GUI_VERTICAL_ALIGNMENT::TOP;
    gui_rect padding;
    auto style_box = style->get_component<gui::style_box>();
    if (style_box) {
        halign = style_box->horizontal_align.value(GUI_HORIZONTAL_ALIGNMENT::LEFT);
        padding = style_box->padding.value(gui_rect(gui::px(0), gui::px(0), gui::px(0), gui::px(0)));                    
    }
    TextLayout::HALIGN tl_halign = convertHorizontalAlignmentToTextLayout(halign);
    TextLayout::VALIGN tl_valign = convertVerticalAlignmentToTextLayout(valign);

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

    gfxm::rect px_padding = gui_to_px(padding, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));
    gfxm::rect px_border = gui_to_px(border_thickness, font->getLineHeight(), gfxm::vec2(text_layout.bounding_width, 0));

    //int client_width = width - (px_padding.min.x + px_padding.max.x + px_border.min.x + px_border.max.x);
    //int client_height = height - (px_padding.min.y + px_padding.max.y + px_border.min.y + px_border.max.y);

    text_layout.setPadding(px_padding.min.x, px_padding.max.x, px_padding.min.y, px_padding.max.y);
    text_layout.setWidth(width_constrained ? ctx.width : std::nullopt);
    text_layout.setHeight(height_constrained ? ctx.height : std::nullopt);
    text_layout.build();

    elem->rc_bounds = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(text_layout.bounding_width, text_layout.bounding_height));
    elem->client_area = elem->rc_bounds;

    width_constrained = true;
    height_constrained = true;
    /*
    text_layout.build(string_utf8, font, client_width);
    
    text_layout.alignHorizontal(tl_halign, client_width);

    text_layout.padHorizontal(px_padding.min.x + px_border.min.x, px_padding.max.x + px_border.max.x);

    text_layout.alignVertical(tl_valign, client_height);
    text_layout.padVertical(px_padding.min.y + px_border.min.y, px_padding.max.y + px_border.max.y);*/
}

