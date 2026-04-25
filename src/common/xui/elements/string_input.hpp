#pragma once

#include "xui/element.hpp"
#include "xui/elements/text_element.hpp"


namespace xui {


    class StringInput : public Element {
        TextElement caption;
        TextElement box;
        std::string value;

    public:
        StringInput();
    };


}

