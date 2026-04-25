#include "string_input.hpp"


namespace xui {


    StringInput::StringInput()
    : caption("String") {
        style_selectors = { "control", "container" };
        size = gui_vec2(gui::fill(), gui::content());

        caption.style_selectors = { "label" };
        caption.size = gui_vec2(gui::perc(25), gui::em(2));
        addToLayout(&caption);

        box.style_selectors = { "input-box-editable" };
        box.size = gui_vec2(gui::fill(), gui::em(2));
        box.same_line = true;
        box.setText("Hello, World!");
        addToLayout(&box);

        setContentTarget(nullptr);
    }


}

