#include "text_element.hpp"
#include "xui/host.hpp"


namespace xui {


    TextElement::TextElement(const char* str) {
        style_selectors = { "label" };
        size = xvec2(content(), content());
        string = str;
    }

    void TextElement::onHitTest(HitResult& hit, int x, int y) {
        if (!gfxm::point_in_rect(gfxm::rect(0, 0, px_size.x, px_size.y), gfxm::vec2(x, y))) {
            return;
        }
        hit.add(HIT::CLIENT, this);
    }

    void TextElement::layout(Host* host, LayoutContext& ctx) {
        for (LAYOUT_PHASE ph = next_layout_phase; ph <= ctx.phase; ++ph) {
            switch (ph) {
            case LAYOUT_PHASE::WIDTH:
                cached_font = host->resolveFont(this);
                if(ctx.px_resolved_width.has_value()) {
                    text_layout.build(string, cached_font, ctx.px_resolved_width.value());
                } else {
                    text_layout.build(string, cached_font, -1);
                }
                ctx.px_measured_width = text_layout.bounding_width;
                next_layout_phase = LAYOUT_PHASE::HEIGHT;
                break;
            case LAYOUT_PHASE::HEIGHT:
                ctx.px_measured_height = text_layout.bounding_height;
                next_layout_phase = LAYOUT_PHASE::COMMIT;
                break;
            case LAYOUT_PHASE::COMMIT:
                px_size = gfxm::ivec2(ctx.px_resolved_width.value(), ctx.px_resolved_height.value());
                next_layout_phase = LAYOUT_PHASE::WIDTH;
                break;
            }
        }
    }

    void TextElement::onDraw(IRenderer* r) {
        //r->drawRectLine(gfxm::rect(0, 0, px_size.x, px_size.y), 0x66FFFFFF);

        std::vector<TextVertex> vertices;
        for (int i = 0; i < text_layout.glyphs.size(); ++i) {
            const auto& g = text_layout.glyphs[i];
            const auto& q = g.makeQuad();

            uint32_t color = g.color;

            // shadow
            const gfxm::vec3 shadow_offs = gfxm::vec3(1.f, -1.f, .0f);
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


}