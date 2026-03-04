#include "gui/elements/element.hpp"

#include "gui/gui_system.hpp"

#include "gui/elements/text_element.hpp"

void GuiElement::_setHost(GuiHost* host) {
    this->host = host;
    for (int i = 0; i < children.size(); ++i) {
        children[i]->_setHost(host);
    }
}

bool GuiElement::isHovered() const {
    //return hasFlags(GUI_FLAG_HOVERED);
    
    if (guiGetMouseCaptor() != 0 && guiGetMouseCaptor() != this) {
        return false;
    }

    auto e = guiGetHoveredElement();
    auto ee = e;
    while (ee) {
        if (ee == this) {
            return true;
        }
        ee = ee->parent;
    }

    return false;
}
bool GuiElement::isPressed() const {
    //return hasFlags(GUI_FLAG_PRESSED);
    return guiGetPressedElement() == this;
}
bool GuiElement::isPulled() const {
    return guiGetPulledElement() == this;
}
bool GuiElement::hasMouseCapture() const {
    return guiGetMouseCaptor() == this;
}



void GuiElement::setParent(GuiElement* elem) {
    setStyleDirty();
    if (parent) {
        parent->_removeChild(this);
    }
    if (!elem) {
        parent = 0;
        return;
    }
    parent = elem->content;
    assert(parent);
    if (parent) {
        parent->children.push_back(this);
        //parent->box.addChild(&this->box);
        parent->sortChildren();

        GUI_MSG_PARAMS params;
        params.setA(this);
        elem->sendMessage(GUI_MSG::CHILD_ADDED, params);
    }
}


void GuiElement::pushBack(const std::string& text) {
    addChild(new GuiTextElement(text));
}
void GuiElement::pushBack(const std::string& text, const std::initializer_list<std::string>& style_classes) {
    auto ptr = new GuiTextElement(text);
    addChild(ptr);
    ptr->setStyleClasses(style_classes);
}


void GuiElement::_addChild(GuiElement* elem) {
    assert(elem != this);
    if (elem->getParent()) {
        elem->getParent()->_removeChild(elem);
    }
    children.push_back(elem);
    //box.addChild(&elem->box);
    elem->parent = this;
    if (elem->owner == 0) {
        elem->owner = this;
    }
    sortChildren();

    GUI_MSG_PARAMS params;
    params.setA(elem);
    this->sendMessage(GUI_MSG::CHILD_ADDED, params);
    // NOTE: Posting this to the queue overflows it when a lot of children change parents
    // which happens quite often
    //guiPostMessage(this, GUI_MSG::CHILD_ADDED, params);

    onInsertChild(elem);
    setStyleDirty();
}
void GuiElement::_removeChild(GuiElement* elem) {
    int id = -1;
    for (int i = 0; i < children.size(); ++i) {
        if (children[i] == elem) {
            id = i;
            break;
        }
    }
    if (id >= 0) {
        onRemoveChild(elem);

        GUI_MSG_PARAMS params;
        params.setA(children[id]);
        this->sendMessage(GUI_MSG::CHILD_REMOVED, params);

        children[id]->parent = 0;
        children.erase(children.begin() + id);
    }

    setStyleDirty();
}


int GuiElement::layoutContentTopDown2(int begin, uint64_t flags) {
    Font* font = getFont();

    auto border_style = getStyleComponent<gui::style_border>();
    auto box_style = getStyleComponent<gui::style_box>();

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
    gfxm::vec2 px_content_margin = gui_to_px(gui_content_margin, font, getClientSize());
    gfxm::rect px_padding = gui_to_px(gui_padding, font, getClientSize());
    gfxm::rect px_border = gui_to_px(gui_border_thickness, font, getClientSize());

    // Unwrap children
    // TODO: Remove this, no need to unwrap after GuiElement::next_wrapped was removed
    std::vector<GuiElement*> children_unwrapped;
    children_unwrapped.reserve(children.size());
    int i = begin;
    for (; i < children.size(); ++i) {
        GuiElement* ch = children[i];
        if (ch->hasFlags(GUI_FLAG_FLOATING)) {
            break;
        }
        if (ch->isHidden()) {
            continue;
        }
        children_unwrapped.push_back(ch);
    }
    int processed_end = i;

    if (hasFlags(GUI_FLAG_HIDE_CONTENT)) {
        return processed_end - begin;
    }

    // ================================
    // Width stage, lines are generated
    // ================================

    if(flags & GUI_LAYOUT_WIDTH_PASS) {
        int client_width = getClientWidth();

        lines.clear();

        i = 0;
        while (i < children_unwrapped.size()) {
            int line_begin = i;
            GuiElement* ch = children_unwrapped[i];
            if (ch->hasFlags(GUI_FLAG_FLOATING)) {
                break;
            }

            int j = i + 1;
            for (; j < children_unwrapped.size(); ++j) {
                ch = children_unwrapped[j];
                if (!ch->hasFlags(GUI_FLAG_SAME_LINE)) {
                    break;
                }
                if (ch->hasFlags(GUI_FLAG_FLOATING)) {
                    break;
                }
            }
            i = j;
            int line_end = i;

            LINE line = { 0 };
            std::vector<BOX>& boxes = line.boxes;
            boxes.reserve(line_end - line_begin);

            for (int j = line_begin; j < line_end; ++j) {
                int li = j - line_begin;
                ch = children_unwrapped[j];

                if (ch->isHidden()) {
                    continue;
                }

                Font* child_font = ch->getFont();
                gui_float width = gui_float_convert(ch->size.x, child_font, client_width);
                gui_float min_width = gui_float_convert(ch->min_size.x, child_font, client_width);
                gui_float max_width = gui_float_convert(ch->max_size.x, child_font, client_width);

                BOX box = { 0 };
                box.overflow_width = gui_float(.0f, gui_pixel);
                box.overflow_height = gui_float(.0f, gui_pixel);
                box.elem = ch;
                box.font = child_font;
                box.index = j;
                box.width = width;
                box.min_width = min_width;
                box.max_width = max_width;
                box.early_layout = false;

                boxes.push_back(box);
            }

            // Adjust width between min and max for pixel values
            {
                for (int j = 0; j < boxes.size(); ++j) {
                    BOX& box = boxes[j];
                    if (box.width.unit == gui_fill) {
                        continue;
                    }

                    const float original = box.width.value;

                    if (box.max_width.unit == gui_pixel) {
                        box.width.value = gfxm::_min(box.max_width.value, box.width.value);
                    }

                    if (box.min_width.unit == gui_pixel) {
                        box.width.value = gfxm::_max(box.min_width.value, box.width.value);
                    }

                    //const float diff = box.width.value - original;
                    //width_eaten += diff;
                    //free_width = gfxm::_max(.0f, free_width - diff);
                }
            }

            // Early layout for horizontal content sizing
            for (int j = 0; j < boxes.size(); ++j) {
                BOX& box = boxes[j];
                GuiElement* ch = boxes[j].elem;
                if (box.width.unit != gui_content) {
                    continue;
                }
                box.early_layout = true;
                ch->layout(
                    gfxm::vec2(0, 0),
                    GUI_LAYOUT_WIDTH_PASS
                    | GUI_LAYOUT_FIT_CONTENT
                );
                box.width.unit = gui_pixel;
                box.width.value = ch->rc_bounds.max.x - ch->rc_bounds.min.x;
            }

            // Inflate client_width to content
            if (flags & GUI_LAYOUT_FIT_CONTENT) {
                int total_width_no_margins = 0;
                for (int j = 0; j < boxes.size(); ++j) {
                    BOX& box = boxes[j];
                    if (box.width.unit == gui_fill) {
                        continue;
                    }
                    total_width_no_margins += box.width.value;
                }
                client_width = gfxm::_max(
                    client_width,
                    int(total_width_no_margins + px_content_margin.x * (boxes.size() - 1))
                );
            }

            // Handle wrapping
            if((flags & GUI_LAYOUT_FIT_CONTENT) == 0) {
                if(/*free_width <= 0 && */ boxes.size() > 0) {
                    int hori_advance = 0;
                    for (int j = 0; j < boxes.size(); ++j) {
                        if (boxes[j].width.unit == gui_pixel) {
                            hori_advance += boxes[j].width.value;
                        }

                        if (hori_advance > client_width && j != 0) {
                            for (int k = j; k < boxes.size(); ++k) {
                                if (boxes[k].width.unit != gui_pixel) {
                                    continue;
                                }
                                //width_eaten -= boxes[k].width.value;
                                //free_width += boxes[k].width.value;
                            }

                            i = boxes[j].index;
                            boxes.resize(j);
                            break;
                        }

                        hori_advance += px_content_margin.x;
                    }
                }
            }

            line.boxes = boxes;
            line.begin = line_begin;
            line.end = line_end;
            lines.push_back(line);
        }

        //
        {
            for (int j = 0; j < lines.size(); ++j) {
                LINE& line = lines[j];

                int n_w_fill = 0;
                int width_eaten = 0;
                int free_width = 0;
                for (int k = 0; k < line.boxes.size(); ++k) {
                    BOX& box = line.boxes[k];
                    if (box.width.unit == gui_fill) {
                        ++n_w_fill;
                    } else {
                        assert(box.width.unit == gui_pixel);
                        width_eaten += box.width.value;
                    }
                }
                free_width = gfxm::_max(0, client_width - width_eaten);

                // Subtract margin space from free_width
                {
                    int margin_space = int(px_content_margin.x * (line.boxes.size() - 1));
                    width_eaten += margin_space;
                    free_width = gfxm::_max(0, free_width - margin_space);
                }

                // Handle min width for fills
                {
                    std::vector<BOX*> boxes_sorted(line.boxes.size());
                    for (int k = 0; k < line.boxes.size(); ++k) {
                        boxes_sorted[k] = &line.boxes[k];
                    }
                    std::sort(boxes_sorted.begin(), boxes_sorted.end(), [](const BOX* a, const BOX* b) {
                        return a->min_width.value > b->min_width.value;
                        });
                    for (int k = 0; k < boxes_sorted.size(); ++k) {
                        BOX& box = *boxes_sorted[k];
                        if (box.width.unit != gui_fill) {
                            continue;
                        }
                        if (box.min_width.unit == gui_fill) {
                            continue;
                        }

                        if (box.min_width.value > free_width / n_w_fill) {
                            --n_w_fill;
                            free_width = gfxm::_max(.0f, free_width - box.min_width.value);
                            box.width = box.min_width;
                        }
                    }
                }

                // Handle max width for fills
                {
                    std::vector<BOX*> boxes_sorted(line.boxes.size());
                    for (int k = 0; k < line.boxes.size(); ++k) {
                        boxes_sorted[k] = &line.boxes[k];
                    }
                    std::sort(boxes_sorted.begin(), boxes_sorted.end(), [](const BOX* a, const BOX* b) {
                        return a->max_width.value < b->max_width.value;
                        });
                    for (int k = 0; k < boxes_sorted.size(); ++k) {
                        BOX& box = *boxes_sorted[k];
                        if (box.width.unit != gui_fill) {
                            continue;
                        }
                        if (box.max_width.unit == gui_fill) {
                            continue;
                        }

                        if (box.max_width.value < free_width / n_w_fill) {
                            --n_w_fill;
                            free_width = gfxm::_max(.0f, free_width - box.max_width.value);
                            box.width = box.max_width;
                        }
                    }
                }

                // Convert fills to pixels (width)
                for (int k = 0; k < line.boxes.size(); ++k) {
                    if (line.boxes[k].width.unit != gui_fill) {
                        continue;
                    }
                    line.boxes[k].width.unit = gui_pixel;
                    line.boxes[k].width.value = free_width / n_w_fill;
                }
            }
        }

        // Call child layouts
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                if (box.early_layout) {
                    continue;
                }

                float w = box.width.value;
                box.elem->layout(
                    gfxm::vec2(w, 0),
                    GUI_LAYOUT_WIDTH_PASS
                );                    
            }
        }

        // Find horizontal content extents
        int max_line_width = 0;
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            int col_cur = 0;
            for (int k = 0; k < line.boxes.size(); ++k) {
                const BOX& box = line.boxes[k];

                int w = box.width.value;
                // Add width first
                col_cur += w;
                // Then calculate new line width
                max_line_width = gfxm::_max(max_line_width, col_cur);
                // Then apply margin before the next sibling
                col_cur += px_content_margin.x;
            }
        }

        rc_content.min.x = client_area.min.x - pos_content.x;
        rc_content.max.x = rc_content.min.x;
        if(lines.size() > 0) {
            rc_content.max.x = client_area.min.x + max_line_width - pos_content.x;
        }

        if (flags & GUI_LAYOUT_FIT_CONTENT) {
            const float sz_content = rc_content.max.x - rc_content.min.x;
            client_area.max.x = gfxm::_max(client_area.max.x, client_area.min.x + sz_content);
            rc_bounds.max.x = client_area.max.x + px_padding.max.x;
        }
    }

    // ================================
    // Height stage
    // ================================
    if(flags & GUI_LAYOUT_HEIGHT_PASS) {
        const int client_height = getClientHeight();

        // Find box heights
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                GuiElement* ch = box.elem;

                Font* child_font = ch->getFont();

                gui_float height = gui_float_convert(ch->size.y, child_font, client_height);
                gui_float min_height = gui_float_convert(ch->min_size.y, child_font, client_height);
                gui_float max_height = gui_float_convert(ch->max_size.y, child_font, client_height);

                box.height = height;
                box.min_height = min_height;
                box.max_height = max_height;
                box.early_layout = false;
            }
        }

        // Early layout for content based height
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                GuiElement* ch = box.elem;
                if (box.height.unit != gui_content) {
                    continue;
                }
                box.early_layout = true;
                ch->layout(
                    gfxm::vec2(0, 0),
                    GUI_LAYOUT_HEIGHT_PASS
                    | GUI_LAYOUT_FIT_CONTENT
                );
                box.height.unit = gui_pixel;
                box.height.value = ch->rc_bounds.max.y - ch->rc_bounds.min.y;
            }
        }

        // Handle min-max height
        // TODO: Not finished, at least fills not handled
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                if (box.min_height.unit == gui_pixel) {
                    box.height.value = gfxm::_max(box.min_height.value, box.height.value);
                }
                if (box.max_height.unit == gui_pixel) {
                    box.height.value = gfxm::_min(box.max_height.value, box.height.value);
                }
            }
        }

        // Find line heights
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            gui_float max_height = gui_float(0, gui_pixel);
            int n_fixed_height = false;
            int n_fill_height = false;
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                gui_float height = box.height;
                if (box.overflow_height.value > height.value) {
                    height = box.overflow_height;
                }

                if (height.unit != gui_fill) {
                    ++n_fixed_height;
                } else {
                    ++n_fill_height;
                }
                if (height.unit != gui_fill && height.value > max_height.value) {
                    max_height = height;
                }
            }
            if (n_fill_height > 0 && n_fixed_height == 0) {
                max_height = gui_float(0, gui_fill);
            }
            line.height = max_height;
        }

        // Find free height
        int n_h_fill = 0;
        int height_eaten = 0;
        int free_height = 0;
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];            
            if (line.height.unit == gui_fill) {
                ++n_h_fill;
            } else {
                assert(line.height.unit == gui_pixel);
                height_eaten += line.height.value;
            }
        }
        free_height = gfxm::_max(0, client_height - height_eaten);

        // Subtract margin space from free_height
        {
            int margin_space = int(px_content_margin.y * (lines.size() - 1));
            height_eaten += margin_space;
            free_height = gfxm::_max(0, free_height - margin_space);
        }

        // Convert height fills to pixels
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            if (line.height.unit == gui_fill) {
                int fill_height = n_h_fill ? free_height / n_h_fill : 0;
                line.height = gui_float(fill_height, gui_pixel);
            }

            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                if (box.height.unit != gui_fill) {
                    continue;
                }
                box.height = line.height;
            }
        }

        // Call child layouts
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                float h = box.height.value;
                if (box.early_layout) {
                    continue;
                }
                box.elem->layout(gfxm::vec2(0,h), GUI_LAYOUT_HEIGHT_PASS);
            }
        }

        // Find vertical content extents
        int total_content_height = 0;
        int row_cur = 0;
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            row_cur += line.height.value;
            total_content_height = row_cur;
            row_cur += px_content_margin.y;
        }

        rc_content.min.y = client_area.min.y - pos_content.y;
        rc_content.max.y = rc_content.min.y;
        if(lines.size() > 0) {
            rc_content.max.y = client_area.min.y + total_content_height - pos_content.y;
        }

        if (flags & GUI_LAYOUT_FIT_CONTENT) {
            const float sz_content = rc_content.max.y - rc_content.min.y;
            client_area.max.y = gfxm::_max(client_area.max.y, client_area.min.y + sz_content);
            rc_bounds.max.y = client_area.max.y + px_padding.max.y;
        }
    }

    // ================================
    // Position stage
    // ================================
    if(flags & GUI_LAYOUT_POSITION_PASS) {
        // Perform child layouts
        int total_content_height = 0;
        int row_cur = 0;
        for (int j = 0; j < lines.size(); ++j) {
            LINE& line = lines[j];
            int col_cur = 0;
            for (int k = 0; k < line.boxes.size(); ++k) {
                BOX& box = line.boxes[k];
                GuiElement* ch = box.elem;

                ch->layout_position
                    = gfxm::vec2(client_area.min.x, client_area.min.y)
                    + gfxm::vec2(col_cur, row_cur)
                    - pos_content;
                assert(
                    box.width.unit == gui_pixel
                    && box.height.unit == gui_pixel
                );
                float w = box.width.value;
                float h = box.height.value;
                ch->layout(
                    gfxm::vec2(w, h),
                    GUI_LAYOUT_POSITION_PASS
                );

                col_cur += w;
                col_cur += px_content_margin.x;
            }
            row_cur += line.height.value;
            total_content_height = row_cur;
            row_cur += px_content_margin.y;
        }
    }

    return processed_end - begin;
}


int GuiElement::update_selection_range(int begin) {
    int last_end = begin;
    for (int i = 0; i < children.size(); ++i) {
        auto ch = children[i];
        last_end = ch->update_selection_range(last_end);
    }
    if (begin == last_end) {
        last_end = begin + self_linear_size;
    }
    linear_begin = begin;
    linear_end = last_end;
    return linear_end;
}
void GuiElement::apply_style() {
    if (needs_style_update) {
        auto& sheet = guiGetStyleSheet();
        GUI_STYLE_FLAGS flags = 0;
        flags |= isHovered() ? GUI_STYLE_FLAG_HOVERED : 0;
        flags |= isPressed() ? GUI_STYLE_FLAG_PRESSED : 0;
        flags |= isSelected() ? GUI_STYLE_FLAG_SELECTED : 0;
        flags |= isFocused() ? GUI_STYLE_FLAG_FOCUSED : 0;
        flags |= isActive() ? GUI_STYLE_FLAG_ACTIVE : 0;
        // TODO: Add more flags
        // Disabled, ReadOnly
        getStyle()->clear();
        sheet.select_styles(getStyle(), style_classes, flags);
        if (getParent()) {
            getStyle()->inherit(*getParent()->getStyle());
        }
        getStyle()->finalize();
    }
    
    for (int i = 0; i < children.size(); ++i) {
        GuiElement* ch = children[i];
        if (ch->is_hidden) {
            continue;
        }
        ch->apply_style();
    }

    needs_style_update = false;
}
void GuiElement::hitTest(GuiHitResult& hit, int x, int y) {
    if (is_hidden) {
        return;
    }

    onHitTest(hit, x - layout_position.x, y - layout_position.y);
}
void GuiElement::layout(const gfxm::vec2& extents, uint64_t flags) {
    if (is_hidden) {
        return;
    }

    onLayout(extents, flags);
}
void GuiElement::draw(int x, int y) {
    if (is_hidden) {
        return;
    }
    //Font* font = getFont();
    //if (font) { guiPushFont(font); }
    guiPushOffset(gfxm::vec2(x, y));
    onDraw();
    guiPopOffset();
    //if (font) { guiPopFont(); }
}
void GuiElement::draw() {
    draw(layout_position.x, layout_position.y);
}


void GuiElement::onHitTest(GuiHitResult& hit, int x, int y) {
    const float resizer_size = 10.f;
    char resizer_mask = 0;
    if ((flags & GUI_FLAG_RESIZE_X) && size.x.unit != gui_fill && size.x.unit != gui_content) {
        resizer_mask |= 0b0010;
    }
    if ((flags & GUI_FLAG_RESIZE_Y) && size.y.unit != gui_fill && size.y.unit != gui_content) {
        resizer_mask |= 0b1000;
    }

    gfxm::rect rc_bounds_extended = rc_bounds;
    if (resizer_mask & 0b0010) {
        rc_bounds_extended.max.x += resizer_size * .5f;
    }
    if (resizer_mask & 0b1000) {
        rc_bounds_extended.max.y += resizer_size * .5f;
    }

    if (!gfxm::point_in_rect(rc_bounds_extended, gfxm::vec2(x, y))) {
        if (hasFlags(GUI_FLAG_MENU_POPUP)) {
            hit.add(GUI_HIT::OUTSIDE_MENU, this);
        }
        return;
    }

    if (flags & GUI_FLAG_RESIZE) {
        guiHitTestResizeBorders(hit, this, rc_bounds, resizer_size, x, y, resizer_mask);
        if (hit.hasHit()) {
            return;
        }
    }

    int i = children.size() - 1;
    // Overlapped
    for (; i >= 0; --i) {
        auto& ch = children[i];
        if (!ch->hasFlags(GUI_FLAG_FLOATING)) {
            break;
        }
        if (ch->isHidden()) {
            continue;
        }
        ch->hitTest(hit, x, y);
        if (hit.hasHit() || ch->hasFlags(GUI_FLAG_BLOCKING)) {
            return;
        }
    }        
    // Content
    if (hasFlags(GUI_FLAG_HIDE_CONTENT)) {
        for (; i >= 0; --i) {
            auto& ch = children[i];
            if (ch->hasFlags(GUI_FLAG_FRAME)) {
                break;
            }
        }
    } else {
        for (; i >= 0; --i) {
            auto& ch = children[i];
            if (ch->hasFlags(GUI_FLAG_FRAME)) {
                break;
            }
            if (ch->isHidden()) {
                continue;
            }
            ch->hitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
    }
    // Frame
    for (; i >= 0; --i) {
        auto& ch = children[i];
        if (ch->isHidden()) {
            continue;
        }
        ch->hitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }
    }

    if (!hasFlags(GUI_FLAG_NO_HIT)) {
        if (hasFlags(GUI_FLAG_CAPTION)) {
            hit.add(GUI_HIT::CAPTION, this);
        } else {
            hit.add(GUI_HIT::CLIENT, this);
        }
    }
    return;
}

bool GuiElement::onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) {
    switch (msg) {
    case GUI_MSG::MOUSE_ENTER: {
        //is_hovered = true;
        setStyleDirty();
        return false;
    }
    case GUI_MSG::MOUSE_LEAVE: {
        //is_hovered = false;
        setStyleDirty();
        return false;
    }
    case GUI_MSG::LBUTTON_DOWN:
    case GUI_MSG::LBUTTON_UP:
        setStyleDirty();
        return false;
    case GUI_MSG::MOUSE_SCROLL: {
            if (!shouldDisplayScroll()) {
                return false;
            }
            int32_t offs = params.getA<int32_t>();
            if (offs > 0) {
                int32_t max_offs = client_area.min.y - rc_content.min.y;
                offs = gfxm::_min(max_offs, offs);
                offs = gfxm::_max(0, offs);
            } else if(offs < 0) {
                offs = gfxm::_max(-int32_t(rc_content.max.y - client_area.max.y), offs);
                offs = gfxm::_min(0, offs);
            }/*
             if (offs == 0) {
             return false;
             }*/
            pos_content.y -= offs;
            return true;
    }
    case GUI_MSG::SB_THUMB_TRACK: {
        return true;
    }
    case GUI_MSG::CLOSE_MENU: {
        if (hasFlags(GUI_FLAG_MENU_POPUP)) {
            setHidden(true);
            return true;
        }
        break;
    }
    case GUI_MSG::LCLICK: {
        if (hasFlags(GUI_FLAG_SELECTABLE)) {
            if (!hasFlags(GUI_FLAG_SELECTED)) {
                addFlags(GUI_FLAG_SELECTED);
            } else {
                removeFlags(GUI_FLAG_SELECTED);
            }
        }
        return false;
    }
    case GUI_MSG::RCLICK:
    case GUI_MSG::DBL_RCLICK: {
        if ((sys_flags & GUI_SYS_FLAG_HAS_CONTEXT_POPUP) != 0) {
            guiShowContextPopup(this, params.getA<int32_t>(), params.getB<int32_t>());
            return true;
        }
        break;
    }
    case GUI_MSG::PULL_START: {
        if (hasFlags(GUI_FLAG_DRAG_CONTENT)) {
            guiCaptureMouse(this);
        }
        return true;
    }
    case GUI_MSG::PULL_STOP: {
        if (hasFlags(GUI_FLAG_DRAG_CONTENT)) {
            guiCaptureMouse(0);
        }
        return true;
    }
    case GUI_MSG::PULL: {
        if (hasFlags(GUI_FLAG_DRAG_CONTENT)) {
            pos_content += gfxm::vec2(
                params.getA<float>(), params.getB<float>()
            );
        }
        return true;
    }
    case GUI_MSG::RESIZING: {
        gfxm::rect* prc = params.getB<gfxm::rect*>();
        switch (params.getA<GUI_HIT>()) {
        case GUI_HIT::LEFT:
            size.x.value = getGlobalBoundingRect().max.x - prc->max.x;
            pos.x = prc->max.x;
            size.x.unit = gui_pixel;
            break;
        case GUI_HIT::RIGHT:
            size.x.value = prc->max.x - getGlobalBoundingRect().min.x;
            size.x.unit = gui_pixel;
            break;
        case GUI_HIT::TOP:
            size.y.value = getGlobalBoundingRect().max.y - prc->max.y;
            pos.y = prc->max.y;
            size.y.unit = gui_pixel;
            break;
        case GUI_HIT::BOTTOM:
            size.y.value = prc->max.y - getGlobalBoundingRect().min.y;
            size.y.unit = gui_pixel;
            break;
        case GUI_HIT::TOPLEFT:
            size.y.value = getGlobalBoundingRect().max.y - prc->max.y;
            pos.y = prc->max.y;
            size.y.unit = gui_pixel;
            size.x.value = getGlobalBoundingRect().max.x - prc->max.x;
            pos.x = prc->max.x;
            size.x.unit = gui_pixel;
            break;
        case GUI_HIT::TOPRIGHT:
            size.y.value = getGlobalBoundingRect().max.y - prc->max.y;
            pos.y = prc->max.y;
            size.y.unit = gui_pixel;
            size.x.value = prc->max.x - getGlobalBoundingRect().min.x;
            size.x.unit = gui_pixel;
            break;
        case GUI_HIT::BOTTOMLEFT:
            size.y.value = prc->max.y - getGlobalBoundingRect().min.y;
            size.y.unit = gui_pixel;
            size.x.value = getGlobalBoundingRect().max.x - prc->max.x;
            pos.x = prc->max.x;
            size.x.unit = gui_pixel;
            break;
        case GUI_HIT::BOTTOMRIGHT:
            size.y.value = prc->max.y - getGlobalBoundingRect().min.y;
            size.y.unit = gui_pixel;
            size.x.value = prc->max.x - getGlobalBoundingRect().min.x;
            size.x.unit = gui_pixel;
            break;
        }
        size.x.value = gfxm::_max(.0f, size.x.value);
        size.y.value = gfxm::_max(.0f, size.y.value);
        return true;
    }
    }
    return false;
}

void GuiElement::onLayout(const gfxm::vec2& extents, uint64_t flags) {
    Font* font = getFont();

    auto border_style = getStyleComponent<gui::style_border>();
    auto box_style = getStyleComponent<gui::style_box>();

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
    gfxm::vec2 px_content_margin = gui_to_px(gui_content_margin, font, getClientSize());
    gfxm::rect px_padding = gui_to_px(gui_padding, font, getClientSize());
    gfxm::rect px_border = gui_to_px(gui_border_thickness, font, getClientSize());

    if (flags & GUI_LAYOUT_WIDTH_PASS) {
        rc_bounds.min.x = 0;
        rc_bounds.max.x = extents.x;
        client_area.min.x = rc_bounds.min.x + tmp_padding.min.x + px_padding.min.x + px_border.min.x;
        client_area.max.x = rc_bounds.max.x - tmp_padding.max.x - px_padding.max.x - px_border.max.x;
    }

    if (flags & GUI_LAYOUT_HEIGHT_PASS) {
        rc_bounds.min.y = 0;
        rc_bounds.max.y = extents.y;
        client_area.min.y = rc_bounds.min.y + tmp_padding.min.y + px_padding.min.y + px_border.min.y;
        client_area.max.y = rc_bounds.max.y - tmp_padding.max.y - px_padding.max.y - px_border.max.y;
    }

    int i = layoutContentTopDown2(0, flags);

    layoutOverlapped(i, flags);
}

void GuiElement::onDraw() {
    // Self
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
    // ----

    guiDrawPushScissorRect(rc_bounds);
    // Frame
    int i = 0;
    for (; i < children.size(); ++i) {
        auto& ch = children[i];
        if (!ch->hasFlags(GUI_FLAG_FRAME)) {
            break;
        }
        if (ch->isHidden()) {
            continue;
        }
        ch->draw();
    }


    // Content
    if (hasFlags(GUI_FLAG_HIDE_CONTENT)) {
        for (; i < children.size(); ++i) {
            auto& ch = children[i];
            if (ch->hasFlags(GUI_FLAG_FLOATING)) {
                break;
            }
        }
    } else {
        if(clip_content) {
            guiDrawPushScissorRect(client_area);
        }
        for (; i < children.size(); ++i) {
            auto ch = children[i];
            if (ch->hasFlags(GUI_FLAG_FLOATING)) {
                break;
            }
            if (ch->isHidden()) {
                continue;
            }
            ch->draw();
        }
        if(clip_content) {
            guiDrawPopScissorRect();
        }
    }

    guiDrawPopScissorRect();

    guiDrawPushScissorRect(client_area);
    // Overlapped
    for (; i < children.size(); ++i) {
        auto& ch = children[i];
        if (ch->isHidden()) {
            continue;
        }
        if (ch->hasFlags(GUI_FLAG_BLOCKING)) {
            guiDrawRect(rc_bounds, 0x77000000);
        }
        ch->draw();
    }
    guiDrawPopScissorRect();

    guiDrawPushScissorRect(rc_bounds);
    if(shouldDisplayScroll()) {
        // Scroll Vertical
        gfxm::rect rc_scroll = client_area;
        rc_scroll.min.x = rc_scroll.max.x + GUI_MARGIN;
        rc_scroll.max.x = rc_scroll.min.x + 10.f;
        //rc_scroll.min.y += GUI_MARGIN;
        //rc_scroll.max.y -= GUI_MARGIN;
        guiDrawRectRound(rc_scroll, 5.f, GUI_COL_HEADER);

        uint32_t col = GUI_COL_BUTTON;/*
        if (pressed_thumb) {
        col = GUI_COL_ACCENT;
        }
        else if (hovered_thumb) {
        col = GUI_COL_BUTTON_HOVER;
        }*/
        float thumb_ratio = 1.f;
        float scroll_to_content_ratio = 1.f;
        float content_height = getContentHeight();
        if (content_height > .0f) {
            thumb_ratio = (client_area.max.y - client_area.min.y) / (rc_content.max.y - rc_content.min.y);
            thumb_ratio = gfxm::_min(1.f, thumb_ratio);
        }
        scroll_to_content_ratio = (rc_scroll.max.y - rc_scroll.min.y) / (rc_content.max.y - rc_content.min.y);
        float scroll_offs = pos_content.y * scroll_to_content_ratio;
        gfxm::rect rc_thumb = rc_scroll;
        rc_thumb.max.y = rc_thumb.min.y + (rc_scroll.max.y - rc_scroll.min.y) * thumb_ratio;
        rc_thumb.min.y += scroll_offs;
        rc_thumb.max.y += scroll_offs;
        guiDrawRectRound(rc_thumb, 5.f, col);
    }
    guiDrawPopScissorRect();
}