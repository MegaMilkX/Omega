#pragma once

#include "xui/element.hpp"
#include "xui/elements/window_decorator.hpp"


namespace xui {


    class WindowLayer : public Element {
        struct Decorator {
            std::unique_ptr<WindowDecorator> deco;
            Element* client_element = nullptr;
        };
        std::vector<Decorator> decorators;
        
        gfxm::ivec2 next_default_pos = gfxm::ivec2(100, 100);

        int findByDecoratorElement(Element*);
        int findByElement(Element*);
    public:
        WindowLayer();

        void onHitTest(HitResult& hit, int x, int y) override;
        void onLayout(Host* host, LayoutContext& ctx) override;
        //void layout(Host*, LayoutContext&) override;
        void onDraw(IRenderer*) override;

        void insert(Element* e);
    };


}

