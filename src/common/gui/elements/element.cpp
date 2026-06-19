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
        ++ref_count;
        parent->children.push_back(this);

        GUI_MSG_PARAMS params;
        params.setA(this);
        elem->sendMessage(GUI_MSG::CHILD_ADDED, params);
    }
}


GuiElement* GuiElement::pushBack(const std::string& text) {
    if (!content) {
        return nullptr;
    }
    auto elem = guiCreate<GuiTextElement>(text);
    addChild(elem);
    return elem;
}
GuiElement* GuiElement::pushBack(const std::string& text, const std::initializer_list<std::string>& style_classes) {
    if (!content) {
        return nullptr;
    }
    auto ptr = guiCreate<GuiTextElement>(text);
    addChild(ptr);
    ptr->setStyleClasses(style_classes);
    return ptr;
}

void GuiElement::popFront() {
    if (!content) {
        return;
    }
    if (content->children.empty()) {
        return;
    }
    content->removeChild(content->children[0]);
}

void GuiElement::_addChild(GuiElement* elem) {
    assert(elem != this);
    if (elem->getParent()) {
        elem->getParent()->_removeChild(elem);
    }
    children.push_back(elem);
    elem->addRef();
    //box.addChild(&elem->box);
    elem->parent = this;
    if (elem->owner == 0) {
        elem->owner = this;
    }

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
        elem->releaseRef();
        onRemoveChild(elem);

        GUI_MSG_PARAMS params;
        params.setA(children[id]);
        this->sendMessage(GUI_MSG::CHILD_REMOVED, params);

        children[id]->parent = 0;
        children.erase(children.begin() + id);
    }

    setStyleDirty();
}


void GuiElement::remove() {
    if (!parent) {
        return;
    }
    parent->_removeChild(this);
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

        {
            auto style_font = getStyle()->get_component<gui::style_font>();
            if (font_cached != style_font->font) {
                // TODO: guiGetDefaultFont()
                onFontChanged(style_font->font.get());
            }
            font_cached = style_font->font;
        }
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

    // Content
    const int content_x = x + pos_content.x;
    const int content_y = y + pos_content.y;
    if (!hasFlags(GUI_FLAG_HIDE_CONTENT)) {
        int i = children.size() - 1;
        for (; i >= 0; --i) {
            auto& ch = children[i];
            if (ch->isHidden()) {
                continue;
            }
            ch->hitTest(hit, content_x, content_y);
            if (hit.hasHit()) {
                return;
            }
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
    case GUI_MSG::MOUSE_SCROLL: {
        if (!shouldDisplayScroll()) {
            return false;
        }
        int32_t offs = params.getA<int32_t>();
        if (offs > 0) {
            int32_t max_offs = client_area.min.y - (rc_content.min.y - pos_content.y);
            offs = gfxm::_min(max_offs, offs);
            offs = gfxm::_max(0, offs);
        } else if(offs < 0) {
            offs = gfxm::_max(-int32_t((rc_content.max.y - pos_content.y) - client_area.max.y), offs);
            offs = gfxm::_min(0, offs);
        }/*
            if (offs == 0) {
            return false;
            }*/
        pos_content.y -= offs;
        return true;
    }
    case GUI_MSG::CLOSE_MENU: {
        if (hasFlags(GUI_FLAG_MENU_POPUP)) {
            setHidden(true);
            return true;
        }
        break;
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

    // Content
    if (!hasFlags(GUI_FLAG_HIDE_CONTENT)) {
        if(clip_content) {
            guiDrawPushScissorRect(rc_bounds);
        }
        guiPushOffset(-gfxm::vec2(pos_content.x, pos_content.y));
        for (int i = 0; i < children.size(); ++i) {
            auto ch = children[i];
            if (ch->isHidden()) {
                continue;
            }
            ch->draw();
        }
        guiPopOffset();
        if(clip_content) {
            guiDrawPopScissorRect();
        }
    }

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

void GuiElement::onFontChanged(Font* fnt) {
    if (layout_handler) {
        layout_handler->fontChanged(this, fnt);
    }
}

int GuiElement::measureWidth(const std::optional<int>& height_constraint) {
    if (!layout_handler) {
        return 0;
    }
    return layout_handler->measureWidth(this, height_constraint);
}
int GuiElement::measureHeight(const std::optional<int>& width_constraint) {
    if (!layout_handler) {
        return 0;
    }
    return layout_handler->measureHeight(this, width_constraint);
}
void GuiElement::layout_2(const gui_layout_context& ctx) {
    if (!layout_handler) {
        return;
    }
    layout_handler->layout(this, ctx);
}

