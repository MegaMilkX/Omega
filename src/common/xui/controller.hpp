#pragma once

#include "xui/element.hpp"


namespace xui {


    class Controller {
        std::unique_ptr<Element> root;
    public:
        Element* getRoot() { return root.get(); }
    };


}