#pragma once

#include "xui/element.hpp"
#include "text_layout/text_layout.hpp"


namespace xui {


    class TextElement : public Element {
        Font* cached_font = nullptr;
        std::string string;
        TextLayout text_layout;
        LAYOUT_PHASE next_layout_phase = LAYOUT_PHASE::WIDTH;

        bool highlighting = false;
        uint32_t highlight_begin = 0;
        uint32_t highlight_end = 0;
    public:
        TextElement(const std::string& str = "");
        void layout(Host*, LayoutContext&) override;
        void onDraw(IRenderer*) override;

        void setText(const std::string& str);
    };


}

