#pragma once

#include "xui/element.hpp"
#include "xui/layout/box_layout.hpp"


namespace xui {


    class IContainer : public Element {
        std::vector<std::unique_ptr<Element>> owned_children;
    public:
        template<typename ELEM_T, typename... ARGS>
        ELEM_T* createItem(ARGS&&... args);
    };


    template<typename ELEM_T, typename... ARGS>
    ELEM_T* IContainer::createItem(ARGS&&... args) {
        ELEM_T* ptr = new ELEM_T(std::forward<ARGS>(args)...);
        owned_children.push_back(std::unique_ptr<Element>(ptr));
        registerChild(ptr);
        return ptr;
    }


}

