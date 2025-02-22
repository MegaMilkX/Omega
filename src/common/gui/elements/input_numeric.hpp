#pragma once

#include <functional>
#include "gui/elements/element.hpp"
#include "gui/elements/label.hpp"
#include "gui/elements/input_numeric_box.hpp"
#include "gui/elements/input.hpp"


class GuiInputNumeric : public GuiElement {
    GuiLabel label;
    GuiInputNumericBox box;

public:
    GuiInputNumeric(
        const char* caption = "InputNumeric",
        unsigned decimal_places = 2,
        bool hexadecimal = false
    ) : label(caption) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        setStyleClasses({ "control" });

        label.setParent(this);
        box.setParent(this);
    }

    std::function<void(float)> on_change;

    void setValue(float value) {
        box.setValue(value);
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        box.onHitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NUMERIC_UPDATE: {
            // TODO
            return true;
        }
        }
        return GuiElement::onMessage(msg, params);
    }

    void onLayout(const gfxm::rect& rc, uint64_t flags) override {        
        setHeight(getFont()->getLineHeight() * 2.f);

        gfxm::rect rc_label = rc;
        gfxm::rect rc_inp;
        guiLayoutSplitRect2XRatio(rc_label, rc_inp, .25f);

        label.layout(rc_label, flags);

        box.layout(rc_inp, flags);

        rc_bounds = label.getBoundingRect();
        gfxm::expand(rc_bounds, box.getBoundingRect());
    }

    void onDraw() override {
        label.draw();
        box.draw();
    }
};

