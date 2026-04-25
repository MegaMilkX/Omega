#include "window_layer.hpp"

#include "xui/host.hpp"
#include "xui/elements/draw_test.hpp"
#include "xui/elements/collapsing_header.hpp"
#include "xui/elements/numeric_input.hpp"
#include "xui/elements/string_input.hpp"
#include "xui/elements/notification_box.hpp"



namespace xui {

    int WindowLayer::findByDecoratorElement(Element* e) {
        int at = -1;
        for (int i = 0; i < decorators.size(); ++i) {
            if (decorators[i].deco.get() == e) {
                at = i;
                break;
            }
        }
        return at;
    }
    int WindowLayer::findByElement(Element* e) {
        int at = -1;
        for (int i = 0; i < decorators.size(); ++i) {
            if (decorators[i].client_element == e) {
                at = i;
                break;
            }
        }
        return at;
    }

    WindowLayer::WindowLayer() {
        {
            auto e = new DrawTest();
            insert(e);
        }
        {
            auto e = new DrawTest();
            e->size = gui_vec2(gui::px(500), gui::px(500));
            insert(e);
        }
        {
            auto root = new Element;
            root->size = gui_vec2(gui::px(300), gui::px(500));
            //root->min_size = gui_vec2(gui::px(200), gui::px(100));
            root->hit_type = HIT::NOWHERE;
            root->style_selectors = { "panel" };

            {
                auto e = new NotificationBox();
                root->addToLayout(e);
            }
            {
                auto collapsing_header = new CollapsingHeader;
                root->addToLayout(collapsing_header);
                {
                    auto e = new NumericInput();
                    collapsing_header->addToLayout(e);
                }
                {
                    auto e = new StringInput();
                    collapsing_header->addToLayout(e);
                }
            }
            {
                auto collapsing_header = new CollapsingHeader;
                root->addToLayout(collapsing_header);
                {                    
                    collapsing_header->addToLayout(new TextElement("0123456789"));
                }
            }

            {
                auto e = new TextElement("The quick brown fox jumps over the lazy dog");
                e->size = gui_vec2(gui::fill(), gui::content());
                root->addToLayout(e);
            }

            insert(root);
        }
        {

            std::string str("The quick brown fox jumps over the lazy dog\nHello, TextLayout!\nBeep boop");
            str += "\n0123456789";
            str += "\nTab \x02\xFF\x66\x77\xFFtest\x03\nABCDEF\t\t\t\t16\t\t\t40\nABC\t\t\t\t\t\t320\t\t\t99";
            str += "\n// TODO: Separate three phase BoxLayout";
            str += "\n// TODO: Generate proper UV values for rounded rectangle";
            str += "\n//\x02\x0F\xCC\x44\xFF DONE\x03: STX/ETX for color";
            str += "\n//\x02\x0F\xCC\x44\xFF DONE\x03: Word wrapping";
            str += "\n//\x02\x0F\xCC\x44\xFF DONE\x03: Alignment";
            str += "\n//\x02\x0F\xCC\x44\xFF DONE\x03: Element::event_table";
            auto e = new TextElement(str.c_str());
            e->style_selectors = { "paragraph" };
            insert(e);
        }
    }

    void WindowLayer::onHitTest(HitResult& hit, int x, int y) {
        if (!gfxm::point_in_rect(gfxm::rect(0, 0, px_size.x, px_size.y), gfxm::vec2(x, y))) {
            return;
        }

        for (int i = decorators.size() - 1; i >= 0; --i) {
            decorators[i].deco->hitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
    }
    void WindowLayer::onLayout(Host* host, LayoutContext& ctx) {        
        switch (ctx.phase) {
        case LAYOUT_PHASE::WIDTH: {
            for (int i = 0; i < decorators.size(); ++i) {
                auto deco = decorators[i].deco.get();
                gui_float width = gui_float_convert(deco->size.x, host->resolveFont(deco), ctx.px_resolved_width.value_or(0));
                LayoutContext ctx_deco{
                    .phase = LAYOUT_PHASE::WIDTH,
                    .px_resolved_width = width.unit == gui_pixel ? width.value : std::optional<int>()
                };
                deco->layout(host, ctx_deco);
                if (width.unit != gui_pixel) {
                    deco->px_size.x = ctx_deco.px_measured_width;
                } else {
                    deco->px_size.x = width.value;
                }
            }

            if (!ctx.px_resolved_width.has_value()) {
                ctx.px_measured_width = 0;
            }
            next_layout_phase = LAYOUT_PHASE::HEIGHT;
            break;
        }
        case LAYOUT_PHASE::HEIGHT: {
            for (int i = 0; i < decorators.size(); ++i) {
                auto deco = decorators[i].deco.get();
                gui_float height = gui_float_convert(deco->size.y, host->resolveFont(deco), ctx.px_resolved_height.value_or(0));
                LayoutContext ctx_deco{
                    .phase = LAYOUT_PHASE::HEIGHT,
                    .px_resolved_width = deco->px_size.x,
                    .px_resolved_height = height.unit == gui_pixel ? height.value : std::optional<int>()
                };
                deco->layout(host, ctx_deco);
                if (height.unit != gui_pixel) {
                    deco->px_size.y = ctx_deco.px_measured_height;
                } else {
                    deco->px_size.y = height.value;
                }
            }

            if (!ctx.px_resolved_height.has_value()) {
                ctx.px_measured_height = 0;
            }
            next_layout_phase = LAYOUT_PHASE::COMMIT;
            break;
        }
        case LAYOUT_PHASE::COMMIT:
            px_size = gfxm::ivec2(ctx.px_resolved_width.value(), ctx.px_resolved_height.value());

            for (int i = 0; i < decorators.size(); ++i) {
                auto deco = decorators[i].deco.get();
                LayoutContext ctx_deco{
                    .phase = LAYOUT_PHASE::COMMIT,
                    .px_resolved_width = deco->px_size.x,
                    .px_resolved_height = deco->px_size.y
                };
                deco->layout(host, ctx_deco);
            }

            next_layout_phase = LAYOUT_PHASE::WIDTH;
            break;
        }
    }

    void WindowLayer::onDraw(IRenderer* r) {
        for (int i = 0; i < decorators.size(); ++i) {
            auto deco = decorators[i].deco.get();
            deco->draw(r);
        }
    }

    void WindowLayer::insert(Element* elem) {
        auto& deco = decorators.emplace_back();
        deco.client_element = elem;
        deco.client_element->size = gui_vec2(gui::fill(), gui::fill()); // TODO: Should be handled by stack layout in decorator's container
        deco.deco.reset(new WindowDecorator);

        deco.deco->px_pos = next_default_pos;
        next_default_pos += gfxm::ivec2(70, 30);

        // eh?
        registerChild(deco.deco.get());
        deco.deco->content_target->addToLayout(deco.client_element);

        deco.deco->subscribe<EvtMove>([this](const EvtMove& evt) {
            auto e = evt.subj;
            int at = findByDecoratorElement(e);
            if (at < 0) {
                return;
            }
            auto deco = decorators[at].deco.get();
            deco->px_pos += gfxm::ivec2(evt.dx, evt.dy);
        });
        deco.deco->subscribe<EvtBringToTop>([this](const EvtBringToTop& evt) {
            auto e = evt.subj;
            int at = findByDecoratorElement(e);
            if (at < 0) {
                return;
            }
            auto deco = std::move(decorators[at]);
            decorators.erase(decorators.begin() + at);
            decorators.emplace_back(std::move(deco));
        });
        deco.deco->subscribe<EvtResizeBegin>([this](EvtResizeBegin& evt) {
            auto e = evt.subj;
            int at = findByDecoratorElement(e);
            if (at < 0) {
                return;
            }
            evt.initial_pos = decorators[at].deco->px_pos;
            evt.initial_size = decorators[at].deco->px_size;
        });
        deco.deco->subscribe<EvtResize>([this](const EvtResize& evt) {
            auto e = evt.subj;
            int at = findByDecoratorElement(e);
            if (at < 0) {
                return;
            }
            decorators[at].deco->px_pos = gfxm::ivec2(evt.rc.min.x, evt.rc.min.y);
            decorators[at].deco->size = gui_vec2(
                gui::px(evt.rc.max.x - evt.rc.min.x),
                gui::px(evt.rc.max.y - evt.rc.min.y)
            );
        });
    }

}

