#include "flow_layout.hpp"

#include "gui/elements/element.hpp"


void GuiFlowLayout::buildLayout(
    GuiElement* elem,
    GuiElement** children,
    size_t child_count,
    std::optional<int> width_constraint,
    std::optional<int> height_constraint,
    BuildMode build_mode
) {
    Font* font = elem->getFont();
    auto style = elem->getStyle();
    auto box_style = style->get_component<gui::style_box>();
    auto border_style = style->get_component<gui::style_border>();

    gui_vec2 gui_content_margin;
    gui_rect gui_padding;
    if (box_style) {
        gui_content_margin = box_style->content_margin.value(gui_vec2());
        gui_padding = box_style->padding.value(gui_rect());
    }
    gui_rect gui_border_thickness;
    if (border_style) {
        gui_border_thickness = gui_rect(
            border_style->thickness_left.value(gui_float(0, gui_pixel)),
            border_style->thickness_top.value(gui_float(0, gui_pixel)),
            border_style->thickness_right.value(gui_float(0, gui_pixel)),
            border_style->thickness_bottom.value(gui_float(0, gui_pixel))
        );
    }

    // TODO: padding and border thickness should not support percent(?) and fill values
    gfxm::vec2 px_content_margin = gui_to_px(gui_content_margin, font, gfxm::vec2());
    gfxm::rect px_padding = gui_to_px(gui_padding, font, gfxm::vec2());
    gfxm::rect px_border = gui_to_px(gui_border_thickness, font, gfxm::vec2());

    // ######################################################

    const GUI_PRIMARY_AXIS primary_axis = elem->primary_axis;
    const std::optional<int>* primary_constraint = nullptr;
    const std::optional<int>* secondary_constraint = nullptr;
    int px_padding_primary_min = 0;
    int px_padding_primary_max = 0;
    int px_padding_secondary_min = 0;
    int px_padding_secondary_max = 0;
    int px_border_primary_min = 0;
    int px_border_primary_max = 0;
    int px_border_secondary_min = 0;
    int px_border_secondary_max = 0;
    int px_spacing_primary = 0;
    int px_spacing_secondary = 0;

    if (primary_axis == GUI_PRIMARY_AXIS::X) {
        primary_constraint = &width_constraint;
        secondary_constraint = &height_constraint;
        px_padding_primary_min = px_padding.min.x;
        px_padding_primary_max = px_padding.max.x;
        px_padding_secondary_min = px_padding.min.y;
        px_padding_secondary_max = px_padding.max.y;
        px_border_primary_min = px_border.min.x;
        px_border_primary_max = px_border.max.x;
        px_border_secondary_min = px_border.min.y;
        px_border_secondary_max = px_border.max.y;
        px_spacing_primary = px_content_margin.x;
        px_spacing_secondary = px_content_margin.y;
    } else {
        primary_constraint = &height_constraint;
        secondary_constraint = &width_constraint;
        px_padding_primary_min = px_padding.min.y;
        px_padding_primary_max = px_padding.max.y;
        px_padding_secondary_min = px_padding.min.x;
        px_padding_secondary_max = px_padding.max.x;
        px_border_primary_min = px_border.min.y;
        px_border_primary_max = px_border.max.y;
        px_border_secondary_min = px_border.min.x;
        px_border_secondary_max = px_border.max.x;
        px_spacing_primary = px_content_margin.y;
        px_spacing_secondary = px_content_margin.x;
    }


    if(dirty_flags & DIRTY_LINES) {
        dirty_flags &= ~DIRTY_LINES;
        dirty_flags |= DIRTY_WRAPPING;

        // Create boxes and measure fixed sides
        boxes.clear();
        for (int i = 0; i < child_count; ++i) {
            GuiElement* child = children[i];
            if (child->isHidden()) {
                continue;
            }
            BOX& box = boxes.emplace_back();
            box.elem = child;
            box.px_width = 0;
            box.px_height = 0;
            box.width_constrained = true;
            box.height_constrained = true;
            switch (child->size.x.unit) {
            case gui_pixel:
                box.px_width = child->size.x.value;
                break;
            case gui_line_height: {
                Font* font = child->getFont();
                box.px_width = child->size.x.value * font->getLineHeight();
                break;
            }
            case gui_percent:
                box.px_width = width_constraint.value_or(0) * child->size.x.value * 0.01f;
                break;
            }
            switch (child->size.y.unit) {
            case gui_pixel:
                box.px_height = child->size.y.value;
                break;
            case gui_line_height: {
                Font* font = child->getFont();
                box.px_height = child->size.y.value * font->getLineHeight();
                break;
            }
            case gui_percent:
                box.px_height = height_constraint.value_or(0) * child->size.y.value * 0.01f;
                break;
            }
        }

        const int secondary_available_space = gfxm::_max(
            0,
            secondary_constraint->value_or(0) - (px_padding_secondary_min + px_padding_secondary_max + px_border_secondary_min + px_border_secondary_max)
        );

        const int secondary_fill = secondary_available_space;
        if (primary_axis == GUI_PRIMARY_AXIS::X) {
            for (int i = 0; i < boxes.size(); ++i) {
                BOX* box = &boxes[i];
                GuiElement* elem = box->elem;
                if (elem->size.y.unit != gui_fill) {
                    continue;
                }
                box->px_height = secondary_fill;
            }
        } else {
            for (int i = 0; i < boxes.size(); ++i) {
                BOX* box = &boxes[i];
                GuiElement* elem = box->elem;
                if (elem->size.x.unit != gui_fill) {
                    continue;
                }
                box->px_width = secondary_fill;
            }
        }

        // Measure all PRIMARY AXIS content sides
        if (primary_axis == GUI_PRIMARY_AXIS::X) {
            for (int i = 0; i < boxes.size(); ++i) {
                BOX* box = &boxes[i];
                GuiElement* elem = box->elem;
                if (elem->size.x.unit != gui_content) {
                    continue;
                }

                if (elem->size.y.unit == gui_content) {
                    box->px_width = elem->measureWidth(std::nullopt);
                    box->px_height = elem->measureHeight(std::nullopt);
                    box->width_constrained = false;
                    box->height_constrained = false;
                } else if (elem->size.y.unit == gui_fill) {
                    if (height_constraint.has_value()) {
                        box->px_height = secondary_fill;
                        box->px_width = elem->measureWidth(secondary_fill);
                        box->width_constrained = false;
                    } else {
                        box->px_width = elem->measureWidth(std::nullopt);
                        box->px_height = elem->measureHeight(std::nullopt);
                        box->width_constrained = false;
                        box->height_constrained = false;
                    }
                } else {
                    box->px_width = elem->measureWidth(box->px_height);
                    box->width_constrained = false;
                }
                box->is_measured = true;
            }
        } else {
            for (int i = 0; i < boxes.size(); ++i) {
                BOX* box = &boxes[i];
                GuiElement* elem = box->elem;
                if (elem->size.y.unit != gui_content) {
                    continue;
                }

                if (elem->size.x.unit == gui_content) {
                    box->px_width = elem->measureWidth(std::nullopt);
                    box->px_height = elem->measureHeight(std::nullopt);
                    box->width_constrained = false;
                    box->height_constrained = false;
                } else if (elem->size.x.unit == gui_fill) {
                    if (width_constraint.has_value()) {
                        box->px_width = secondary_fill;
                        box->px_height = elem->measureHeight(secondary_fill);
                        box->height_constrained = false;
                    } else {
                        box->px_width = elem->measureWidth(std::nullopt);
                        box->px_height = elem->measureHeight(std::nullopt);
                        box->width_constrained = false;
                        box->height_constrained = false;
                    }
                } else {
                    box->px_height = elem->measureHeight(box->px_width);
                    box->height_constrained = false;
                }
                box->is_measured = true;
            }
        }

        int primary_axis_length_eaten = 0;
        if (primary_axis == GUI_PRIMARY_AXIS::X) {
            for (int i = 0; i < boxes.size(); ++i) {
                BOX* box = &boxes[i];
                primary_axis_length_eaten += box->px_width;
            }
        } else {
            for (int i = 0; i < boxes.size(); ++i) {
                BOX* box = &boxes[i];
                primary_axis_length_eaten += box->px_height;
            }
        }

        int primary_fill_count = 0;
        for (int i = 0; i < boxes.size(); ++i) {
            BOX* box = &boxes[i];
            gui_float* side = primary_axis == GUI_PRIMARY_AXIS::X ? &box->elem->size.x : &box->elem->size.y;
            if (side->unit == gui_fill) {
                ++primary_fill_count;
            }
        }
        int primary_fill = 0;
        if(primary_fill_count > 0) {
            int primary_spacing_sum = px_spacing_primary * (boxes.size() - 1);
            primary_fill = gfxm::_max(
                0,
                primary_constraint->value_or(0) - primary_axis_length_eaten - (primary_spacing_sum + px_padding_primary_min + px_padding_primary_max + px_border_primary_min + px_border_primary_max)
            ) / primary_fill_count;
            for (int i = 0; i < boxes.size(); ++i) {
                BOX* box = &boxes[i];
                GuiElement* elem = box->elem;
                gui_float* size_primary = primary_axis == GUI_PRIMARY_AXIS::X ? &elem->size.x : &elem->size.y;
                if (size_primary->unit != gui_fill) {
                    continue;
                }
                int* px_primary = primary_axis == GUI_PRIMARY_AXIS::X ? &box->px_width : &box->px_height;
                *px_primary = primary_fill;
            }
        }

        // Measure all leftover SECONDARY AXIS content sides
        if (primary_axis == GUI_PRIMARY_AXIS::X) {
            for (int i = 0; i < boxes.size(); ++i) {
                BOX* box = &boxes[i];
                GuiElement* elem = box->elem;
                if (box->is_measured) {
                    continue;
                }
                if (elem->size.y.unit != gui_content) {
                    continue;
                }

                if (elem->size.x.unit == gui_content) {
                    box->px_width = elem->measureWidth(std::nullopt);
                    box->px_height = elem->measureHeight(std::nullopt);
                    box->width_constrained = false;
                    box->height_constrained = false;
                } else if (elem->size.x.unit == gui_fill) {
                    if (width_constraint.has_value()) {
                        box->px_height = elem->measureHeight(primary_fill);
                        box->px_width = primary_fill;
                        box->height_constrained = false;
                    } else {
                        box->px_width = elem->measureWidth(std::nullopt);
                        box->px_height = elem->measureHeight(std::nullopt);
                        box->width_constrained = false;
                        box->height_constrained = false;
                    }
                } else {
                    box->px_height = elem->measureHeight(box->px_width);
                    box->height_constrained = false;
                }
                box->is_measured = true;
            }
        } else {
            for (int i = 0; i < boxes.size(); ++i) {
                BOX* box = &boxes[i];
                GuiElement* elem = box->elem;
                if (box->is_measured) {
                    continue;
                }
                if (elem->size.x.unit != gui_content) {
                    continue;
                }

                if (elem->size.y.unit == gui_content) {
                    box->px_width = elem->measureWidth(std::nullopt);
                    box->px_height = elem->measureHeight(std::nullopt);
                    box->width_constrained = false;
                    box->height_constrained = false;
                } else if (elem->size.y.unit == gui_fill) {
                    if (height_constraint.has_value()) {
                        box->px_width = elem->measureWidth(primary_fill);
                        box->px_height = primary_fill;
                        box->width_constrained = false;
                    } else {
                        box->px_width = elem->measureWidth(std::nullopt);
                        box->px_height = elem->measureHeight(std::nullopt);
                        box->width_constrained = false;
                        box->height_constrained = false;
                    }
                } else {
                    box->px_width = elem->measureWidth(box->px_height);
                    box->width_constrained = false;
                }
                box->is_measured = true;
            }
        }
    }

    if (primary_axis == GUI_PRIMARY_AXIS::X) {
        if (build_mode == BuildMode::TellHeight) {
            return;
        }
    } else {
        if (build_mode == BuildMode::TellWidth) {
            return;
        }
    }

    if (dirty_flags & DIRTY_WRAPPING) {
        dirty_flags &= ~DIRTY_WRAPPING;
        dirty_flags |= DIRTY_PLACEMENT;

        // Wrapping
        wrapped_lines.clear();
        if(primary_constraint->has_value() && elem->hasFlags(GUI_FLAG_ENABLE_WRAPPING)) {
            for (int i = 0; i < boxes.size(); ++i) {
                int px_advance = primary_axis == GUI_PRIMARY_AXIS::X ? boxes[i].px_width : boxes[i].px_height;
                int primary_axis_advance = px_advance;
                int j = i + 1;
                for (; j < boxes.size(); ++j) {
                    BOX* box = &boxes[j];
                    px_advance = primary_axis == GUI_PRIMARY_AXIS::X ? box->px_width : box->px_height;
                    if (primary_axis_advance + px_advance > primary_constraint->value()) {
                        break;
                    }
                    primary_axis_advance += px_advance;
                }
                LINE* line = &wrapped_lines.emplace_back();
                line->begin = i;
                line->end = j;
                i = j - 1;
            }
        } else {
            LINE* line = &wrapped_lines.emplace_back();
            line->begin = 0;
            line->end = boxes.size();
        }

        // Measure line extents
        bounding_width = 0;
        bounding_height = 0;
        int& primary_bounding_size = primary_axis == GUI_PRIMARY_AXIS::X ? bounding_width : bounding_height;
        int& secondary_bounding_size = primary_axis == GUI_PRIMARY_AXIS::X ? bounding_height : bounding_width;

        // primary axis
        for (int i = 0; i < wrapped_lines.size(); ++i) {
            LINE* line = &wrapped_lines[i];
            int line_length = 0;
            for (int j = line->begin; j < line->end; ++j) {
                BOX* box = &boxes[j];
                int box_primary_size = primary_axis == GUI_PRIMARY_AXIS::X ? box->px_width : box->px_height;
                line_length += box_primary_size;
            }
            int& ln_primary_size = primary_axis == GUI_PRIMARY_AXIS::X ? line->px_width : line->px_height;

            int primary_spacing_sum = px_spacing_primary * (line->end - line->begin - 1);
            line_length += primary_spacing_sum;
            ln_primary_size = line_length;
            primary_bounding_size = gfxm::_max(primary_bounding_size, line_length);
        }
        primary_bounding_size += px_padding_primary_min + px_padding_primary_max + px_border_primary_min + px_border_primary_max;

        // secondary axis
        for (int i = 0; i < wrapped_lines.size(); ++i) {
            LINE* line = &wrapped_lines[i];
            int line_thickness = 0;
            for (int j = line->begin; j < line->end; ++j) {
                BOX* box = &boxes[j];
                int box_secondary_size = primary_axis == GUI_PRIMARY_AXIS::X ? box->px_height : box->px_width;
                line_thickness = gfxm::_max(line_thickness, box_secondary_size);
            }
            int& ln_secondary_size = primary_axis == GUI_PRIMARY_AXIS::X ? line->px_height : line->px_width;
            ln_secondary_size = line_thickness;
            secondary_bounding_size += line_thickness;
        }
        int secondary_spacing_sum = px_spacing_secondary * (wrapped_lines.size() - 1);
        secondary_extent_no_padding = secondary_bounding_size + secondary_spacing_sum;
        secondary_bounding_size = secondary_extent_no_padding + px_padding_secondary_min + px_padding_secondary_max + px_border_secondary_min + px_border_secondary_max;
    }


    if (primary_axis == GUI_PRIMARY_AXIS::X) {
        if (build_mode == BuildMode::TellWidth) {
            return;
        }
    } else {
        if (build_mode == BuildMode::TellHeight) {
            return;
        }
    }

    // Expand cross-axis fills
    if(!secondary_constraint->has_value()) {
        if (primary_axis == GUI_PRIMARY_AXIS::X) {
            for (int i = 0; i < wrapped_lines.size(); ++i) {
                LINE* line = &wrapped_lines[i];
                for (int j = line->begin; j < line->end; ++j) {
                    BOX* box = &boxes[j];
                    if (box->elem->size.y.unit != gui_fill) {
                        continue;
                    }
                    box->px_height = gfxm::_max(box->px_height, line->px_height);
                    box->height_constrained = true;
                }
            }
        } else {
            for (int i = 0; i < wrapped_lines.size(); ++i) {
                LINE* line = &wrapped_lines[i];
                for (int j = line->begin; j < line->end; ++j) {
                    BOX* box = &boxes[j];
                    if (box->elem->size.x.unit != gui_fill) {
                        continue;
                    }
                    box->px_width = gfxm::_max(box->px_width, line->px_width);
                    box->width_constrained = true;
                }
            }
        }
    }

    // Placement
    if(dirty_flags & DIRTY_PLACEMENT) {
        dirty_flags &= ~DIRTY_PLACEMENT;
        
        GUI_VERTICAL_ALIGNMENT valign = GUI_VERTICAL_ALIGNMENT::TOP;
        GUI_HORIZONTAL_ALIGNMENT halign = GUI_HORIZONTAL_ALIGNMENT::LEFT;
        GUI_INLINE_ALIGNMENT inline_align = GUI_INLINE_ALIGNMENT::MIN;
        if (box_style) {
            valign = box_style->vertical_align.value(GUI_VERTICAL_ALIGNMENT::TOP);
            halign = box_style->horizontal_align.value(GUI_HORIZONTAL_ALIGNMENT::LEFT);
            inline_align = box_style->inline_align.value(GUI_INLINE_ALIGNMENT::MIN);
        }

        float valign_mul = guiVerticalAlignToFloat(valign);
        float halign_mul = guiHorizontalAlignToFloat(halign);

        float primary_align_mul = .0f;
        float secondary_align_mul = .0f;
        const float inline_align_mul = guiInlineAlignToFloat(inline_align);
        if (primary_axis == GUI_PRIMARY_AXIS::X) {
            primary_align_mul = guiHorizontalAlignToFloat(halign);
            secondary_align_mul = guiVerticalAlignToFloat(valign);
        } else {
            primary_align_mul = guiVerticalAlignToFloat(valign);
            secondary_align_mul = guiHorizontalAlignToFloat(halign);
        }

        const int primary_bounding_size = primary_axis == GUI_PRIMARY_AXIS::X ? bounding_width : bounding_height;
        const int secondary_bounding_size = primary_axis == GUI_PRIMARY_AXIS::X ? bounding_height : bounding_width;
        const int primary_align_space
            = (primary_constraint->has_value() ? primary_constraint->value() : primary_bounding_size)
            - (px_padding_primary_min + px_padding_primary_max + px_border_primary_min + px_border_primary_max);

        const int secondary_align_space
            = (secondary_constraint->has_value() ? secondary_constraint->value() : secondary_bounding_size)
            - (px_padding_secondary_min + px_padding_secondary_max + px_border_secondary_min + px_border_secondary_max);
        const int secondary_align_offset = (secondary_align_space - secondary_extent_no_padding) * secondary_align_mul;

        int x_cur = 0;
        int y_cur = 0;
        int& primary_cur = primary_axis == GUI_PRIMARY_AXIS::X ? x_cur : y_cur;
        int& secondary_cur = primary_axis == GUI_PRIMARY_AXIS::X ? y_cur : x_cur;
        secondary_cur += secondary_align_offset + px_padding_secondary_min + px_border_secondary_min;
        for (int i = 0; i < wrapped_lines.size(); ++i) {
            LINE* line = &wrapped_lines[i];

            const int line_primary_size = primary_axis == GUI_PRIMARY_AXIS::X ? line->px_width : line->px_height;
            const int primary_align_offset = (primary_align_space - line_primary_size) * primary_align_mul;

            const int line_secondary_size = primary_axis == GUI_PRIMARY_AXIS::X ? line->px_height : line->px_width;

            primary_cur = primary_align_offset + px_padding_primary_min + px_border_primary_min;
            for (int j = line->begin; j < line->end; ++j) {
                BOX* box = &boxes[j];
                const int box_primary_size = primary_axis == GUI_PRIMARY_AXIS::X ? box->px_width : box->px_height;
                const int box_secondary_size = primary_axis == GUI_PRIMARY_AXIS::X ? box->px_height : box->px_width;
                const int inline_align_offset = (line_secondary_size - box_secondary_size) * inline_align_mul;

                int x_cur_tmp = x_cur;
                int y_cur_tmp = y_cur;
                int* secondary_cur = primary_axis == GUI_PRIMARY_AXIS::X ? &y_cur_tmp : &x_cur_tmp;
                *secondary_cur += inline_align_offset;

                GuiElement* elem = box->elem;
                elem->layout_position = gfxm::vec2(x_cur_tmp, y_cur_tmp);
                primary_cur += box_primary_size + px_spacing_primary;
            }
            secondary_cur += line_secondary_size + px_spacing_secondary;
        }
    }

    // TODO: invalidate the first stage for now
    // need to track content, font, padding, content margin, child flags, child sizing changes to remove this
    dirty_flags = DIRTY_LINES;
}

void GuiFlowLayout::fontChanged(GuiElement* elem, Font* font) {
    // TODO: invalidate cache
}
int GuiFlowLayout::measureWidth(GuiElement* elem, const std::optional<int>& height_constraint) {
    buildLayout(elem, elem->children.data(), elem->children.size(), std::nullopt, height_constraint, BuildMode::TellWidth);
    return bounding_width;
}

int GuiFlowLayout::measureHeight(GuiElement* elem, const std::optional<int>& width_constraint) {
    buildLayout(elem, elem->children.data(), elem->children.size(), width_constraint, std::nullopt, BuildMode::TellHeight);
    return bounding_height;
}

void GuiFlowLayout::layout(GuiElement* elem, const gui_layout_context& ctx) {
    elem->rc_bounds = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(ctx.width.value_or(bounding_width), ctx.height.value_or(bounding_height)));
    elem->client_area = elem->rc_bounds;
    elem->rc_content = gfxm::rect(gfxm::vec2(0, 0), gfxm::vec2(bounding_width, bounding_height));

    buildLayout(elem, elem->children.data(), elem->children.size(), ctx.width, ctx.height, BuildMode::Full);

    for (int i = 0; i < boxes.size(); ++i) {
        BOX* box = &boxes[i];
        GuiElement* child = box->elem;
        child->layout_2(gui_layout_context{
            box->width_constrained ? std::optional<int>(box->px_width) : std::nullopt,
            box->height_constrained ? std::optional<int>(box->px_height) : std::nullopt
        });
    }
}

