#pragma once

#include "xui/elements/container.hpp"
#include "xui/layout/box_layout.hpp"


namespace xui {


    class StackElement : public IContainer {
        LAYOUT_PHASE next_layout_phase;
        BoxLayout box_layout;
        std::vector<BoxLayout::Box> boxes;
    public:
        StackElement();
        void onHitTest(HitResult& hit, int x, int y) override;
        void layout(Host*, LayoutContext&) override;
        void onDraw(IRenderer*) override;

        void addToLayout(Element*);
    };


}

