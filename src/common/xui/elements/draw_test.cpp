#include "draw_test.hpp"


namespace xui {


    void DrawTest::onHitTest(HitResult& hit, int x, int y) {
        if (!gfxm::point_in_rect(gfxm::rect(0, 0, px_size.x, px_size.y), gfxm::vec2(x, y))) {
            return;
        }
        hit.add(HIT::CLIENT, this);
    }
    void DrawTest::layout(Host*, LayoutContext& ctx) {
        px_size = gfxm::ivec2(ctx.px_resolved_width.value(), ctx.px_resolved_height.value());
    }
    void DrawTest::onDraw(IRenderer* r) {
        static float time = .0f;
        time += .004f;
        int segments = 64;

        gfxm::vec3 center(px_size.x * .5f, px_size.y * .5f, .0f);
        gfxm::vec3 scale(px_size.x * .5f, px_size.y * .5f, .0f);
        gfxm::vec3 scale2(.5f, .5f, .0f);

        std::vector<Vertex> vertices;
        for (int i = 0; i <= segments; ++i) {
            float f = i / float(segments);
            float f2pi = f * gfxm::pi * 2.f;
            float f4pi = f * gfxm::pi * 4.f;
            float f8pi = f * gfxm::pi * 8.f;
            gfxm::vec3 pt(cosf(f2pi), sinf(f2pi), .0f);
            gfxm::vec3 toffs(cosf(f4pi + time), sinf(f4pi + time), .0f);
            toffs *= 100.f;
            gfxm::vec3 toffs2(sinf(gfxm::pi + f4pi + time * 1.5f), cosf(gfxm::pi + f4pi + time * 1.5f), .0f);
            toffs2 *= 100.f;
            vertices.push_back(Vertex{ (pt * scale + toffs + toffs2) * scale2 + center, gfxm::vec2(0, 0), gfxm::hsv2rgb32(f, .3f, 1.f, 1.f) });
        }
        r->drawLineStrip(vertices.data(), vertices.size());
    }


}