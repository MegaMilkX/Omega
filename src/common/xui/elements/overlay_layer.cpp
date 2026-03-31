#include "overlay_layer.hpp"

// TESTING
#include "xui/elements/text_element.hpp"

namespace xui {


    OverlayLayer::OverlayLayer() {
        hit_type = HIT::NOWHERE;
        // TESTING
        {
            auto e = createItem<TextElement>("Hello, World!");
            e->size = xvec2(fill(), content());
        }
        {
            auto e = createItem<TextElement>("FooBar");
            e->size = xvec2(content(), content());
            e->same_line = true;
        }
        {
            auto e = createItem<TextElement>("TextElement test.");
            e->size = xvec2(content(), content());
        }/*
        {
            auto e = createItem<TextElement>("Beep Boop.");
            e->size = xvec2(fill(), fill());
        }*/
        {
            auto e = createItem<TextElement>("Here's\nsome\nmultiline\nbullshit");
            e->size = xvec2(content(), content());
        }
    }


}

