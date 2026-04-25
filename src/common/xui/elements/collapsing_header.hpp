#pragma once

#include "xui/element.hpp"
#include "xui/elements/text_element.hpp"


namespace xui {


    class CollapsingHeader : public Element {
        TextElement icon_arrow;
        TextElement icon_close;
        TextElement caption;
        Element header;
        Element content;
    public:
        CollapsingHeader();
    };


}

