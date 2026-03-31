#pragma once

#include "xui/element.hpp"
#include "text_layout/text_layout.hpp"


namespace xui {


    class TextElement : public Element {
        Font* cached_font = nullptr;
        std::string string;
        TextLayout text_layout;
        LAYOUT_PHASE next_layout_phase = LAYOUT_PHASE::WIDTH;
    public:
        TextElement(const char* str = "");
        void onHitTest(HitResult& hit, int x, int y) override;
        void layout(Host*, LayoutContext&) override;
        void onDraw(IRenderer*) override;
    };


}

