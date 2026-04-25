#include "overlay_layer.hpp"

// TESTING
#include "xui/elements/text_element.hpp"

namespace xui {


    OverlayLayer::OverlayLayer() {
        hit_type = HIT::NOWHERE;
        // TESTING
        {
            auto e = createItem<TextElement>("Hello, World!");
            e->size = gui_vec2(gui::fill(), gui::content());
        }
        {
            auto e = createItem<TextElement>("FooBar");
            e->size = gui_vec2(gui::content(), gui::content());
            e->same_line = true;
        }
        {
            auto e = createItem<TextElement>("TextElement test.");
            e->size = gui_vec2(gui::content(), gui::content());
        }/*
        {
            auto e = createItem<TextElement>("Beep Boop.");
            e->size = gui_vec2(fill(), fill());
        }*/
        {
            auto e = createItem<TextElement>("Forced\nline\nbreaks");
            e->size = gui_vec2(gui::content(), gui::content());
        }
    }


}

