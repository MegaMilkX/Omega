#pragma once

#include <functional>
#include "gui/elements/element.hpp"
#include "gui/elements/label.hpp"
#include "gui/elements/input_string_box.hpp"

class GuiInputString : public GuiElement {
protected:
    GuiLabel label;
    GuiInputStringBox box;

public:
    GuiInputString(
        const char* caption = "InputString"
    ) : label(caption) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        setStyleClasses({ "control" });

        label.setParent(this);
        box.setParent(this);
    }

    std::function<void(const std::string&)> on_change;

    void setValue(const std::string& value) {
        if(value != box.getValue()) {
            box.setValue(value);
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NUMERIC_UPDATE: {
            if (on_change) {
                on_change(box.getValue());
            }
            return true;
        }
        }
        return GuiElement::onMessage(msg, params);
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        box.onHitTest(hit, x, y);
        if (hit.hasHit()) {
            return;
        }
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