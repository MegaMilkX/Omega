#include "window_layer.hpp"


namespace xui {


    WindowLayer::WindowLayer() {
        {
            auto e = new TextElement("WindowContent");
            insert(e);
        }
        {
            auto ee = new StackElement();
            ee->size = xvec2(fill(), fill());
            ee->createItem<TextElement>("Hello, World!");
            ee->createItem<TextElement>("Test");
            insert(ee);
        }
    }

    void WindowLayer::onHitTest(HitResult& hit, int x, int y) {
        if (!gfxm::point_in_rect(gfxm::rect(0, 0, px_size.x, px_size.y), gfxm::vec2(x, y))) {
            return;
        }

        for (int i = decorators.size() - 1; i >= 0; --i) {
            decorators[i]->hitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
    }

    void WindowLayer::layout(Host* host, LayoutContext& ctx) {
        px_size = gfxm::ivec2(ctx.px_resolved_width.value(), ctx.px_resolved_height.value());

        gfxm::ivec2 next_pos(100, 100);
        for (int i = 0; i < decorators.size(); ++i) {
            auto d = decorators[i].get();
            d->px_pos = next_pos;

            LayoutContext ctx{
                .phase = LAYOUT_PHASE::COMMIT,
                .px_resolved_width = 400,
                .px_resolved_height = 700
            };
            d->layout(host, ctx);

            next_pos += gfxm::ivec2(100, 50);
        }
    }

    void WindowLayer::onDraw(IRenderer* r) {
        //r->drawRectLine(gfxm::rect(0, 0, px_size.x, px_size.y), 0x66FFFFFF);
        for (int i = 0; i < decorators.size(); ++i) {
            auto d = decorators[i].get();
            d->draw(r);
        }
    }

    void WindowLayer::insert(Element* elem) {
        auto deco = new WindowDecorator;
        decorators.push_back(std::unique_ptr<WindowDecorator>(deco));
        deco->setContent(elem);
        registerChild(deco);
    }

}

