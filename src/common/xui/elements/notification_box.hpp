#pragma once

#include "xui/element.hpp"
#include "xui/elements/text_element.hpp"


namespace xui {


    class NotificationBox : public Element {
        TextElement icon;
        TextElement caption;

    public:
        NotificationBox();
    };


}

