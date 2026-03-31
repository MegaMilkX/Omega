#pragma once

#include "xui/element.hpp"
#include "xui/elements/window_layer.hpp"
#include "xui/elements/overlay_layer.hpp"


namespace xui {


    class Root : public Element {
        WindowLayer window_layer;
        OverlayLayer overlay_layer;
    public:
        Root();
        void onHitTest(HitResult& hit, int x, int y) override;
        void layout(Host*, LayoutContext&) override;
        void onDraw(IRenderer*) override;
    };


}

