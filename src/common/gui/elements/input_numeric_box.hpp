#pragma once

#include "gui/elements/text_element.hpp"


class GuiInputNumericBox : public GuiTextElement {
    gfxm::vec2 mouse_pos;
    float value = .0f;
    bool is_value_dirty = true;

    bool is_editing = false;
    bool is_dragging = false;

    void setValueDirty() {
        is_value_dirty = true;
    }

public:
    GuiInputNumericBox() {
        setSize(gui::fill(), gui::em(2));

        setStyleClasses({ "input-box" });

        inner_alignment = HORIZONTAL_ALIGNMENT::CENTER;
    }

    void updateView() {
        setContent(std::format("{:.2f}", value));
        is_value_dirty = false;
    }
    void updateFromView() {
        std::string str = getText();
        LOG_DBG("updateFromView(): " << str);
        size_t idx = 0;
        if(!str.empty()) {
            value = std::stof(str, &idx);
        } else {
            value = .0f;
        }
        setValueDirty();
        if (getParent()) {
            getParent()->sendMessage(GUI_MSG::NUMERIC_UPDATE, GUI_MSG_PARAMS());
        }
    }

    void setValue(float value) {
        this->value = value;
        setValueDirty();
    }
    float getValue() const {
        return value;
    }

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
            setStyleClasses({ "input-box" });
            updateFromView();
            break;
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK: {
            if(!is_editing && !is_dragging) {
                is_editing = true;
                setStyleClasses({ "input-box", "input-box-editable" });
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

                    if (getParent()) {
                        getParent()->sendMessage(GUI_MSG::NUMERIC_UPDATE, GUI_MSG_PARAMS());
                    }
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
    
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        if (is_value_dirty) {
            updateView();
        }
        GuiTextElement::onLayout(extents, flags);
    }
};

