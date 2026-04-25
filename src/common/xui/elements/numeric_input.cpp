#include "numeric_input.hpp"


namespace xui {

    void NumericInput::updateBoxes() {
        for(int i = 0; i < sizeof(boxes) / sizeof(boxes[0]); ++i) {
            auto& box = boxes[i];
            box.setText(std::format("{:.2f}", values[i]));
        }
    }

    NumericInput::NumericInput()
    : caption("Numeric") {
        style_selectors = { "control", "container" };
        size = gui_vec2(gui::fill(), gui::content());

        caption.style_selectors = { "label" };
        caption.size = gui_vec2(gui::perc(25), gui::em(2));
        addToLayout(&caption);

        for(int i = 0; i < sizeof(boxes) / sizeof(boxes[0]); ++i) {
            auto& box = boxes[i];
            box.setBehaviorFlags(BHV_FLAG_CAPTURE_ON_MOUSE_DOWN);
            box.style_selectors = { "input-box" };
            box.size = gui_vec2(gui::fill(), gui::em(2));
            box.same_line = true;
            box.setText("");
            box.subscribe<EvtDrag>([this, i](const EvtDrag& e) {
                values[i] += e.dx * .01f;
                updateBoxes();
            });
            addToLayout(&box);
        }

        setContentTarget(nullptr);

        updateBoxes();
    }


}

