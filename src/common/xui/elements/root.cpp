#include "root.hpp"


namespace xui {


    Root::Root() {
        style_selectors = { "root" };
        registerChild(&overlay_layer);
        registerChild(&window_layer);
    }

    void Root::onHitTest(HitResult& hit, int x, int y) {
        if (!gfxm::point_in_rect(gfxm::rect(0, 0, px_size.x, px_size.y), gfxm::vec2(x, y))) {
            return;
        }

        overlay_layer.hitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }

        window_layer.hitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }
    }

    void Root::layout(Host* host, LayoutContext& ctx) {
        px_size = gfxm::ivec2(ctx.px_resolved_width.value(), ctx.px_resolved_height.value());

        LayoutContext ctx_windows = ctx;
        window_layer.layout(host, ctx_windows);

        LayoutContext ctx_overlay = ctx;
        overlay_layer.layout(host, ctx_overlay);
    }

    void Root::onDraw(IRenderer* r) {
        window_layer.draw(r);
        overlay_layer.draw(r);
    }


}