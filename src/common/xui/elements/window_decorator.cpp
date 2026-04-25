#include "window_decorator.hpp"


namespace xui {
    static void calcResizeBorders(
            const gfxm::rect& rect,
            float thickness_outer, float thickness_inner,
            gfxm::rect* left, gfxm::rect* right, gfxm::rect* top, gfxm::rect* bottom
        ) {
        assert(left && right && top && bottom);

        left->min = gfxm::vec2(
            rect.min.x - thickness_outer,
            rect.min.y - thickness_outer
        );
        left->max = gfxm::vec2(
            rect.min.x + thickness_inner,
            rect.max.y + thickness_outer
        );

        right->min = gfxm::vec2(
            rect.max.x - thickness_inner,
            rect.min.y - thickness_outer
        );
        right->max = gfxm::vec2(
            rect.max.x + thickness_outer,
            rect.max.y + thickness_outer
        );

        top->min = gfxm::vec2(
            rect.min.x - thickness_outer,
            rect.min.y - thickness_outer
        );
        top->max = gfxm::vec2(
            rect.max.x + thickness_outer,
            rect.min.y + thickness_inner
        );

        bottom->min = gfxm::vec2(
            rect.min.x - thickness_outer,
            rect.max.y - thickness_inner
        );
        bottom->max = gfxm::vec2(
            rect.max.x + thickness_outer,
            rect.max.y + thickness_outer
        );
    }
    static void hitTestResizeBorders(HitResult& hit, Element* who, const gfxm::rect& rc, float border_thickness, int x, int y, char mask) {
        gfxm::vec2 pt(x, y);
        gfxm::rect rc_szleft, rc_szright, rc_sztop, rc_szbottom;
        calcResizeBorders(rc, border_thickness * .5f, border_thickness * .5f, &rc_szleft, &rc_szright, &rc_sztop, &rc_szbottom);
        char sz_flags = 0b0000;
        if (gfxm::point_in_rect(rc_szleft, pt)) {
            sz_flags |= 0b0001;
        } else if (gfxm::point_in_rect(rc_szright, pt)) {
            sz_flags |= 0b0010;
        }
        if (gfxm::point_in_rect(rc_sztop, pt)) {
            sz_flags |= 0b0100;
        } else if (gfxm::point_in_rect(rc_szbottom, pt)) {
            sz_flags |= 0b1000;
        }
        sz_flags &= mask;
        HIT ht = HIT::ERR;
        switch (sz_flags) {
        case 0b0001: ht = HIT::LEFT; break;
        case 0b0010: ht = HIT::RIGHT; break;
        case 0b0100: ht = HIT::TOP; break;
        case 0b1000: ht = HIT::BOTTOM; break;
        case 0b0101: ht = HIT::TOPLEFT; break;
        case 0b1001: ht = HIT::BOTTOMLEFT; break;
        case 0b0110: ht = HIT::TOPRIGHT; break;
        case 0b1010: ht = HIT::BOTTOMRIGHT; break;
        };
        if (ht != HIT::ERR) {
            hit.add(ht, who);
            return;
        }
        return;
    }

    WindowDecorator::WindowDecorator()
    : title("Window") {
        size = gui_vec2(gui::px(300), gui::px(450));
        hit_type = HIT::CAPTION;

        style_selectors = { "xui-window-frame" };

        addToLayout(&title);
        title.style_selectors = { "window-title" };
        title.size = gui_vec2(gui::fill(), gui::em(2));
        title.hit_type = HIT::CAPTION;
        addToLayout(&content);
        content.hit_type = HIT::NOWHERE;
        content.size = gui_vec2(gui::fill(), gui::fill());
        content.draw_flags |= DRAW_FLAG_CLIP_CONTENT;
        setContentTarget(&content);
    }

    void WindowDecorator::onHitTest(HitResult& hit, int x, int y) {
        hitTestResizeBorders(hit, this, gfxm::rect(0, 0, px_size.x, px_size.y), 6, x, y, 0b1111);
        if (hit.hasHit()) {
            return;
        }
        Element::onHitTest(hit, x, y);
    }

}