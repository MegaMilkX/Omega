#include "stack_element.hpp"
#include "xui/host.hpp"

// TESTING
#include "xui/elements/text_element.hpp"


namespace xui {


    StackElement::StackElement()
        : next_layout_phase(LAYOUT_PHASE::WIDTH) {
    }

    void StackElement::onHitTest(HitResult& hit, int x, int y) {
        if (!gfxm::point_in_rect(gfxm::rect(0, 0, px_size.x, px_size.y), gfxm::vec2(x, y))) {
            return;
        }

        for (int i = 0; i < layout_children.size(); ++i) {
            layout_children[i]->hitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }

        if (hit_type != HIT::NOWHERE) {
            hit.add(hit_type, this);
        }
    }

    void StackElement::layout(Host* host, LayoutContext& ctx) {
        for (LAYOUT_PHASE ph = next_layout_phase; ph <= ctx.phase; ++ph) {
            switch (ph) {
            case LAYOUT_PHASE::WIDTH:
                if(ctx.px_resolved_width.has_value()) {
                    box_layout.m_px_container_width = ctx.px_resolved_width.value();
                    box_layout.m_fit_content_width = false;
                } else {
                    // TODO: BoxLayout needs to resolve content size and be able to accept max sizes(?)
                    box_layout.m_px_container_width = 0;
                    box_layout.m_fit_content_width = true;
                }

                // TODO: TEMPORARY CONTENT MARGIN
                box_layout.content_margin = px(10, 10);

                boxes.clear();
                for (int i = 0; i < layout_children.size(); ++i) {
                    auto ch = layout_children[i];
                    Font* font = host->resolveFont(ch);
                    boxes.push_back(
                        BoxLayout::Box{
                            .user_ptr = ch,
                            .px_line_height = font->getLineHeight(),
                            .size = ch->size,
                            .min_size = ch->min_size,
                            .max_size = ch->max_size,
                            .same_line = ch->same_line
                        }
                    );
                }
                box_layout.buildWidth(boxes.data(), boxes.size(), [host](const BoxLayout::BOX* box)->int {
                    Element* e = (Element*)(box->elem->user_ptr);
                    LayoutContext ctx{
                        .phase = LAYOUT_PHASE::WIDTH,
                    };
                    e->layout(host, ctx);
                    return ctx.px_measured_width;
                });
                ctx.px_measured_width = box_layout.m_px_container_width;
                next_layout_phase = LAYOUT_PHASE::HEIGHT;
                break;
            case LAYOUT_PHASE::HEIGHT:
                if(ctx.px_resolved_height.has_value()) {
                    box_layout.m_px_container_height = ctx.px_resolved_height.value();
                    box_layout.m_fit_content_height = false;
                } else {
                    // TODO: BoxLayout needs to resolve content size and be able to accept max sizes
                    box_layout.m_px_container_height = 1000;
                    box_layout.m_fit_content_height = true;
                }
                box_layout.buildHeight([host](const BoxLayout::BOX* box)->int {
                    Element* e = (Element*)(box->elem->user_ptr);
                    LayoutContext ctx{
                        .phase = LAYOUT_PHASE::HEIGHT,
                        .px_resolved_width = int(box->width.value),
                    };
                    e->layout(host, ctx);
                    return ctx.px_measured_height;
                });
                ctx.px_measured_height = box_layout.m_px_container_height;
                next_layout_phase = LAYOUT_PHASE::COMMIT;
                break;
            case LAYOUT_PHASE::COMMIT:
                px_size = gfxm::ivec2(ctx.px_resolved_width.value(), ctx.px_resolved_height.value());

                box_layout.buildPosition();
                for (int i = 0; i < box_layout.lines.size(); ++i) {
                    auto& line = box_layout.lines[i];
                    for(int j = 0; j < line.boxes.size(); ++j) {
                        auto& box = line.boxes[j];
                        int px_resolved_width = box.width.value;
                        int px_resolved_height = box.height.value;
                        Element* e = (Element*)(box.elem->user_ptr);
                        LayoutContext ctx{
                            .phase = LAYOUT_PHASE::COMMIT,
                            .px_resolved_width = px_resolved_width,
                            .px_resolved_height = px_resolved_height
                        };
                        e->layout(host, ctx);
                        e->px_pos = gfxm::ivec2(box.px_pos_x, box.px_pos_y);
                    }
                }
                next_layout_phase = LAYOUT_PHASE::WIDTH;
                break;
            }
        }
    }

    void StackElement::onDraw(IRenderer* r) {
        //r->drawRectLine(gfxm::rect(0, 0, px_size.x, px_size.y), 0x66FFFFFF);
        for (int i = 0; i < layout_children.size(); ++i) {
            auto ch = layout_children[i];
            ch->draw(r);
        }
    }

    void StackElement::addToLayout(Element* e) {
        registerChild(e);
    }

}

