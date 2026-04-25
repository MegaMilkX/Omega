#pragma once

#include "xui/element.hpp"
#include "xui/elements/text_element.hpp"


namespace xui {


    class NumericInput : public Element {
        TextElement caption;
        TextElement boxes[4];
        gfxm::vec4 values;

        void updateBoxes();
    public:
        NumericInput();
    };


}

