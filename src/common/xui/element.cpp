#include "element.hpp"
#include "xui/host.hpp"
#include "gui/style/styles.hpp"

#include "xui/layout/box_layout_handler.hpp"
#include "xui/layout/float_layout_handler.hpp"
#include "xui/layout/stack_layout_handler.hpp"


namespace xui {


    void Element::registerChild(Element* e) {
        if (e->layout_parent) {
            e->layout_parent->unregisterChild(e);
        }
        e->layout_parent = this;
        layout_children.push_back(e);
        e->is_style_dirty = true;
        e->addRef();
    }

    void Element::unregisterChild(Element* e) {
        auto it = std::find(layout_children.begin(), layout_children.end(), e);
        if (it == layout_children.end()) {
            return;
        }
        layout_children.erase(it);
        e->layout_parent = nullptr;
        e->is_style_dirty = true;
        e->releaseRef();
    }

    void Element::setContentTarget(Element* e) {
        content_target = e;
    }


    Element::~Element() {
        if (layout_parent) {
            layout_parent->unregisterChild(this);
        }
        for (auto* ch : layout_children) {
            ch->layout_parent = nullptr;
        }

        // TODO: elem_hovered dangling
    }

    void Element::onHitTest(HitResult& hit, int x, int y) {
        if (!gfxm::point_in_rect(gfxm::rect(0, 0, px_size.x, px_size.y), gfxm::vec2(x, y))) {
            return;
        }

        // TODO: Should be configurable front to back or back to front
        for (int i = 0; i < layout_children.size(); ++i) {
            if (layout_children[i]->isHidden()) {
                continue;
            }
            layout_children[i]->hitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }

        hit.add(hit_type, this);
    }
    void Element::onLayout(Host* host, LayoutContext& ctx) {
        switch (ctx.phase) {
        case LAYOUT_PHASE::WIDTH: {
            if (layout_mode != LayoutNone && (!layout_handler || layout_handler->getType() != layout_mode)) {
                switch (layout_mode) {
                case LayoutNone:
                    layout_handler.reset(); // if mode is LayoutNone, it will reset every time due to layout_handler being null
                    break;
                case LayoutBox:
                    layout_handler.reset(new BoxLayoutHandler);
                    break;
                default:
                    assert(false && "LayoutHandler not implemented");
                    layout_handler.reset();
                }
            }
            if (!layout_handler) {
                return;
            }
            layout_handler->init(this);
            int px_measured = layout_handler->resolveWidth(host, ctx.px_resolved_width.value_or(-1));
            if (!ctx.px_resolved_width.has_value()) {
                ctx.px_measured_width = px_measured;
            }
            next_layout_phase = LAYOUT_PHASE::HEIGHT;
            break;
        }
        case LAYOUT_PHASE::HEIGHT: {
            if (!layout_handler) {
                return;
            }
            int px_measured = layout_handler->resolveHeight(host, ctx.px_resolved_height.value_or(-1));
            if (!ctx.px_resolved_height.has_value()) {
                ctx.px_measured_height = px_measured;
            }
            next_layout_phase = LAYOUT_PHASE::COMMIT;
            break;
        }
        case LAYOUT_PHASE::COMMIT:
            px_size = gfxm::ivec2(ctx.px_resolved_width.value(), ctx.px_resolved_height.value());
            if (!layout_handler) {
                return;
            }
            layout_handler->resolvePlacement(host);
            next_layout_phase = LAYOUT_PHASE::WIDTH;
            break;
        }
    }
    void Element::layout(Host* host, LayoutContext& ctx) {
        const LAYOUT_PHASE last_phase = ctx.phase;
        for (LAYOUT_PHASE ph = next_layout_phase; ph <= last_phase; ++ph) {
            ctx.phase = ph;
            onLayout(host, ctx);
        }
    }
    void Element::onDraw(IRenderer* renderer) {
        Font* font = resolved_font;
        assert(font);
        auto style = getStyle();

        auto style_bg_color = style->get_component<gui::style_background_color>();
        auto style_border = style->get_component<gui::style_border>();
        auto style_border_radius = style->get_component<gui::style_border_radius>();

        float rc_width = px_size.x;
        float rc_height = px_size.y;
        float min_side = gfxm::_min(rc_width, rc_height);
        float half_min_side = min_side * .5f;
        float br_tl = 0, br_tr = 0, br_bl = 0, br_br = 0;
        float bt_l = 0, bt_t = 0, bt_r = 0, bt_b = 0;
        if (style_border_radius) {
            br_tl = gui_to_px(style_border_radius->radius_top_left.value(), font, half_min_side);
            br_tr = gui_to_px(style_border_radius->radius_top_right.value(), font, half_min_side);
            br_bl = gui_to_px(style_border_radius->radius_bottom_left.value(), font, half_min_side);
            br_br = gui_to_px(style_border_radius->radius_bottom_right.value(), font, half_min_side);
        }
        if (style_border) {
            bt_l = gui_to_px(style_border->thickness_left.value(), font, rc_width * .5f);
            bt_t = gui_to_px(style_border->thickness_top.value(), font, rc_height * .5f);
            bt_r = gui_to_px(style_border->thickness_right.value(), font, rc_width * .5f);
            bt_b = gui_to_px(style_border->thickness_bottom.value(), font, rc_height * .5f);
        }

        if (style_bg_color) {
            if (style_border_radius) {
                renderer->drawRectRound(
                    gfxm::rect(0, 0, px_size.x, px_size.y),
                    style_bg_color->color.value(),
                    br_tl, br_tr, br_bl, br_br
                );
            } else {
                renderer->drawRect(
                    gfxm::rect(0, 0, px_size.x, px_size.y),
                    style_bg_color->color.value()
                );
            }
        }
        if (style_border) {
            if (style_border_radius) {
                renderer->drawRectRoundBorder(
                    gfxm::rect(0, 0, px_size.x, px_size.y),
                    style_border->color_left.value(), // TODO: all the color sides
                    br_tl, br_tr, br_bl, br_br,
                    bt_l, bt_t, bt_r, bt_b
                );
            } else {
                renderer->drawRectBorder(
                    gfxm::rect(0, 0, px_size.x, px_size.y),
                    style_border->color_left.value(), // TODO: all the color sides
                    bt_l, bt_t, bt_r, bt_b
                );
            }
        }

        // Children
        if(draw_flags & DRAW_FLAG_CLIP_CONTENT) {
            renderer->pushClipRect(0, 0, px_size.x, px_size.y);
        }
        for (int i = 0; i < layout_children.size(); ++i) {
            auto ch = layout_children[i];
            if (ch->isHidden()) {
                continue;
            }
            ch->draw(renderer);
        }
        if(draw_flags & DRAW_FLAG_CLIP_CONTENT) {
            renderer->popClipRect();
        }
    }


    void Element::applyStyle(Host* host) {
        if (is_style_dirty) {
            auto& sheet = host->getStyleSheet();
            GUI_STYLE_FLAGS flags = 0;
            flags |= isHovered() ? GUI_STYLE_FLAG_HOVERED : 0;
            flags |= isPressed() ? GUI_STYLE_FLAG_PRESSED : 0;
            flags |= isSelected() ? GUI_STYLE_FLAG_SELECTED : 0;
            flags |= isFocused() ? GUI_STYLE_FLAG_FOCUSED : 0;
            flags |= isActive() ? GUI_STYLE_FLAG_ACTIVE : 0;
            flags |= isDisabled() ? GUI_STYLE_FLAG_DISABLED : 0;
            flags |= isReadOnly() ? GUI_STYLE_FLAG_READONLY : 0;

            getStyle()->clear();
            sheet.select_styles(getStyle(), style_selectors, flags);
            if (layout_parent) {
                getStyle()->inherit(*layout_parent->getStyle());
            }
            getStyle()->finalize();
            resolved_font = host->resolveFont(this);
        }

        for (int i = 0; i < layout_children.size(); ++i) {
            Element* ch = layout_children[i];
            // TODO
            /*if (ch->is_hidden) {
                continue;
            }*/
            ch->applyStyle(host);
        }

        is_style_dirty = false;
    }

    void Element::hitTest(HitResult& hit, int x, int y) {
        onHitTest(hit, x - px_pos.x, y - px_pos.y);
    }

    void Element::draw(IRenderer* r) {
        r->pushOffset(gfxm::vec2(px_pos.x, px_pos.y));
        onDraw(r);
        r->popOffset();
    }

    void Element::addToLayout(Element* e) {
        if (!content_target) {
            return;
        }
        content_target->registerChild(e);
    }

    gfxm::ivec2 Element::globalToLocal(const gfxm::ivec2& pos) const {
        gfxm::ivec2 offs;
        const Element* e = this;
        while (e) {
            offs += gfxm::ivec2(e->px_pos.x, e->px_pos.y);
            e = e->layout_parent;
        }
        return pos - offs;
    }
    gfxm::ivec2 Element::getGlobalPos() const {
        gfxm::ivec2 offs;
        const Element* e = this;
        while (e) {
            offs += gfxm::ivec2(e->px_pos.x, e->px_pos.y);
            e = e->layout_parent;
        }
        return offs;
    }
    gfxm::rect Element::getGlobalRect() const {
        gfxm::vec2 offs;
        const Element* e = this;
        while (e) {
            offs += gfxm::vec2(e->px_pos.x, e->px_pos.y);
            e = e->layout_parent;
        }
        return gfxm::rect(offs.x, offs.y, offs.x + px_size.x, offs.y + px_size.y);
    }
    gui::style* Element::getStyle() {
        if (!style) {
            style.reset(new gui::style);
        }
        return style.get();
    }
    Element* Element::getContentTarget() {
        return content_target;
    }

    void Element::setHidden(bool value) {
        is_hidden = value;
    }
    bool Element::isHidden() const {
        return is_hidden;
    }

    void Element::setBehaviorFlags(behavior_flags_t flags) {
        behavior_flags |= flags;
    }
    void Element::clearBehaviorFlags(behavior_flags_t flags) {
        behavior_flags &= ~flags;
    }

    void Element::setDisplayFlags(display_flags_t flags) {
        display_flags |= flags;
        is_style_dirty = true;
    }
    void Element::clearDisplayFlags(display_flags_t flags) {
        display_flags &= ~flags;
        is_style_dirty = true;
    }


}

