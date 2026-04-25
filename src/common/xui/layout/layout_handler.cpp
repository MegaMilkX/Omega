#include "layout_handler.hpp"


namespace xui {


    void LayoutHandler::init(Element* elem) {
        element = elem;
        onInit(elem);
    }


}