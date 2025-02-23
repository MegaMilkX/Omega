#pragma once

#include <functional>
#include "gui/elements/element.hpp"
#include "gui/elements/label.hpp"
#include "gui/elements/input_numeric_box.hpp"
#include "gui/elements/input.hpp"


template<int COUNT>
class GuiInputNumericBase : public GuiElement {
protected:
    GuiLabel label;
    GuiInputNumericBox boxes[COUNT];

public:
    GuiInputNumericBase(
        const char* caption = "InputNumeric",
        unsigned decimal_places = 2,
        bool hexadecimal = false
    ) : label(caption) {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);
        setStyleClasses({ "control" });

        label.setParent(this);
        for (int i = 0; i < COUNT; ++i) {
            boxes[i].setParent(this);
        }
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        for (int i = 0; i < COUNT; ++i) {
            boxes[i].onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
    }
    
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        setHeight(getFont()->getLineHeight() * 2.f);
        gfxm::rect rc_label = rc;
        gfxm::rect rc_inp;
        guiLayoutSplitRect2XRatio(rc_label, rc_inp, .25f);

        label.layout(rc_label, flags);

        std::vector<gfxm::rect> rcs(COUNT);
        guiLayoutSplitRectH(rc_inp, &rcs[0], rcs.size(), 3);
        for (int i = 0; i < COUNT; ++i) {
            boxes[i].layout(rcs[i], flags);
        }

        rc_bounds = label.getBoundingRect();
        for (int i = 0; i < COUNT; ++i) {
            gfxm::expand(rc_bounds, boxes[i].getBoundingRect());
        }
    }

    void onDraw() override {
        label.draw();
        for (int i = 0; i < COUNT; ++i) {
            boxes[i].draw();
        }
    }
};

class GuiInputNumeric : public GuiInputNumericBase<1> {
public:
    GuiInputNumeric(
        const char* caption = "InputNumeric",
        unsigned decimal_places = 2,
        bool hexadecimal = false
    ) : GuiInputNumericBase<1>(caption, decimal_places, hexadecimal)
    {}

    std::function<void(float)> on_change;

    void setValue(float value) {
        if(value != boxes[0].getValue()) {
            boxes[0].setValue(value);
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NUMERIC_UPDATE: {
            if (on_change) {
                on_change(boxes[0].getValue());
            }
            return true;
        }
        }
        return GuiElement::onMessage(msg, params);
    }
};

class GuiInputNumeric2 : public GuiInputNumericBase<2> {
public:
    GuiInputNumeric2(
        const char* caption = "InputNumeric",
        unsigned decimal_places = 2,
        bool hexadecimal = false
    ) : GuiInputNumericBase<2>(caption, decimal_places, hexadecimal)
    {}

    std::function<void(float, float)> on_change;

    void setValue(float x, float y) {
        if(x != boxes[0].getValue()) {
            boxes[0].setValue(x);
        }
        if(y != boxes[1].getValue()) {
            boxes[1].setValue(y);
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NUMERIC_UPDATE: {
            if (on_change) {
                on_change(boxes[0].getValue(), boxes[1].getValue());
            }
            return true;
        }
        }
        return GuiElement::onMessage(msg, params);
    }
};

class GuiInputNumeric3 : public GuiInputNumericBase<3> {
public:
    GuiInputNumeric3(
        const char* caption = "InputNumeric",
        unsigned decimal_places = 2,
        bool hexadecimal = false
    ) : GuiInputNumericBase<3>(caption, decimal_places, hexadecimal)
    {}

    std::function<void(float, float, float)> on_change;

    void setValue(float x, float y, float z) {
        if(x != boxes[0].getValue()) {
            boxes[0].setValue(x);
        }
        if(y != boxes[1].getValue()) {
            boxes[1].setValue(y);
        }
        if(z != boxes[2].getValue()) {
            boxes[2].setValue(z);
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NUMERIC_UPDATE: {
            if (on_change) {
                on_change(boxes[0].getValue(), boxes[1].getValue(), boxes[2].getValue());
            }
            return true;
        }
        }
        return GuiElement::onMessage(msg, params);
    }
};

class GuiInputNumeric4 : public GuiInputNumericBase<4> {
public:
    GuiInputNumeric4(
        const char* caption = "InputNumeric",
        unsigned decimal_places = 2,
        bool hexadecimal = false
    ) : GuiInputNumericBase<4>(caption, decimal_places, hexadecimal)
    {}

    std::function<void(float, float, float, float)> on_change;

    void setValue(float x, float y, float z, float w) {
        if(x != boxes[0].getValue()) {
            boxes[0].setValue(x);
        }
        if(y != boxes[1].getValue()) {
            boxes[1].setValue(y);
        }
        if(z != boxes[2].getValue()) {
            boxes[2].setValue(z);
        }
        if(w != boxes[3].getValue()) {
            boxes[3].setValue(w);
        }
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::NUMERIC_UPDATE: {
            if (on_change) {
                on_change(boxes[0].getValue(), boxes[1].getValue(), boxes[2].getValue(), boxes[3].getValue());
            }
            return true;
        }
        }
        return GuiElement::onMessage(msg, params);
    }
};