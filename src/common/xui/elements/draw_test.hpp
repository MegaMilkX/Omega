#pragma once

#include "xui/element.hpp"


namespace xui {


    class DrawTest : public Element {
    public:
        void onHitTest(HitResult& hit, int x, int y) override;
        void layout(Host*, LayoutContext&) override;
        void onDraw(IRenderer*) override;
    };


}

