#pragma once

#include "xui/element.hpp"
#include "xui/elements/window_decorator.hpp"


namespace xui {


    class WindowLayer : public Element {
        std::vector<std::unique_ptr<WindowDecorator>> decorators;
    public:
        WindowLayer();
        void onHitTest(HitResult& hit, int x, int y) override;
        void layout(Host*, LayoutContext&) override;
        void onDraw(IRenderer*) override;

        void insert(Element* e);
    };


}

