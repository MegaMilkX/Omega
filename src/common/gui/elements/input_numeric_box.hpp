#pragma once

#include "gui/elements/text_element.hpp"


class GuiInputNumericBox : public GuiTextElement {
    gfxm::vec2 mouse_pos;
    float value = .0f;
    bool is_view_dirty = false;

    bool is_editing = false;
    bool is_dragging = false;

    void setViewDirty() {
        is_view_dirty = true;
    }

public:
    GuiInputNumericBox() {
        setSize(0, 0);
        setMaxSize(0, 0);
        setMinSize(0, 0);

        setStyleClasses({ "input-box" });

        inner_alignment = TEXT_ALIGNMENT::CENTER;
    }

    void updateView() {
        setContent(std::to_string(value));
        is_view_dirty = false;
    }
    void updateFromView() {
        std::string str = getText();
        LOG_DBG("updateFromView(): " << str);
        size_t idx = 0;
        value = std::stof(str, &idx);
        setViewDirty();
    }

    void setValue(float value) {
        this->value = value;
        setViewDirty();
    }
    /*
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if(!is_editing) {
            if (gfxm::point_in_rect(rc_bounds, gfxm::vec2(x, y))) {
                hit.add(GUI_HIT::CLIENT, this);
                return;
            }
        } else {
            GuiElement::onHitTest(hit, x, y);
        }
    }*/
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::UNICHAR: {
            switch (params.getA<GUI_CHAR>()) {
            case GUI_CHAR::RETURN: {
                is_editing = false;
                updateFromView();
                guiUnfocusWindow(this);
                return true;
            }
            }
            break;
        }
        case GUI_MSG::FOCUS:
            if (is_editing) {
                break;
            } else {
                return false;
            }
        case GUI_MSG::UNFOCUS:
            is_editing = false;
            updateFromView();
            break;
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK: {
            if(!is_editing && !is_dragging) {
                is_editing = true;
                guiSetFocusedWindow(this);
                guiSetHighlight(linear_begin, linear_end - 1/* -1 for ETX */);
            }
            if (is_dragging) {
                is_dragging = false;
            }
            return true;
        }
        case GUI_MSG::MOUSE_MOVE: {
            if (guiHasMouseCapture(this)) {
                gfxm::vec2 new_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
                if(!is_dragging) {
                    float diff = fabsf(new_mouse_pos.x - mouse_pos.x);
                    if (diff > 10.f) {
                        is_dragging = true;
                        mouse_pos = guiGetMousePos();
                    }
                } else {
                    gfxm::vec2 new_mouse_pos = gfxm::vec2(params.getA<int32_t>(), params.getB<int32_t>());
                    setValue(value + .01f * (new_mouse_pos.x - mouse_pos.x));
                    mouse_pos = new_mouse_pos;
                }
            }
            return true;
        }
        case GUI_MSG::LBUTTON_DOWN:
            if(!is_editing) {
                guiCaptureMouse(this);
                mouse_pos = guiGetMousePos();
                return true;
            } else {
                break;
            }
        case GUI_MSG::LBUTTON_UP:
            guiReleaseMouseCapture(this);
            return true;
        }
        return GuiTextElement::onMessage(msg, params);
    }
    
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        if (is_view_dirty) {
            updateView();
        }
        GuiTextElement::onLayout(rc, flags);
    }
};

