#pragma once

#include "xui/element.hpp"
#include "xui/elements/text_element.hpp"
#include "xui/elements/stack_element.hpp"


namespace xui {


    class WindowDecorator : public StackElement {
        TextElement title;
        StackElement content;
    public:
        WindowDecorator();
        void onDraw(IRenderer*) override;

        void setContent(Element* e);
    };


}

