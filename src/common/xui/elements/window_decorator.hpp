#pragma once

#include "xui/element.hpp"
#include "xui/elements/text_element.hpp"


namespace xui {


    class WindowDecorator : public Element {
        TextElement title;
        Element content;
    public:
        WindowDecorator();
        void onHitTest(HitResult& hit, int x, int y) override;
    };


}

