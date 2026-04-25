#pragma once

#include "layout_handler.hpp"
#include "box_layout.hpp"


namespace xui {


    class BoxLayoutHandler : public LayoutHandler {
        BoxLayout layout;
        std::vector<BoxLayout::Box> boxes;
    public:
        BoxLayoutHandler()
            : LayoutHandler(LayoutBox) {}

        void onInit(Element* elem) override;
        int resolveWidth(Host*, int width_available) override;
        int resolveHeight(Host*, int height_available) override;
        void resolvePlacement(Host*) override;
    };

}

