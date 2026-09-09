#pragma once

#include "gui/elements/text_element.hpp"
#include "gui/gui_system.hpp"


class GuiInputNumericBox : public GuiTextElement {
    gfxm::vec2 mouse_pos;
    float value = .0f;
    int decimal_places = 2;
    bool is_value_dirty = true;

    bool is_editing = false;
    bool is_dragging = false;

    void setValueDirty() {
        is_value_dirty = true;
    }

public:
    GuiInputNumericBox(int decimal_places = 2)
    : decimal_places(decimal_places) {
        setReadOnly(false);
        setSize(gui::fill(), gui::em(2));
        setStyleClasses({ "input-box" });

        subscribe<GuiEvt_Focus>([this](const GuiEvt_Focus& e) {
            if (is_editing) {
                e.new_focused = this;
                e.invoke_next();
            } else {
                e.consume = false;
            }
        });
        subscribe<GuiEvt_Unfocus>([this](const GuiEvt_Unfocus& e) {
            is_editing = false;
            setStyleClasses({ "input-box" });
            updateFromView();
            e.invoke_next();
        });

        subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick&) {
            if(!is_editing && !is_dragging) {
                is_editing = true;
                setStyleClasses({ "input-box", "input-box-editable" });
                guiSetFocusedWindow(this);
                guiSetHighlight(linear_begin, linear_end - 1/* -1 for ETX */);
            }
            is_dragging = false;
        });

        subscribe<GuiEvt_MouseBtn>([this](const GuiEvt_MouseBtn& e) {
            if (e.btn == GUI_MOUSE_LEFT) {
                if (e.state == GUI_KEY_DOWN) {
                    if(!is_editing) {
                        guiCaptureMouse(this);
                        mouse_pos = guiGetMousePos();
                    } else {
                        e.invoke_next();
                    }
                } else if (e.state == GUI_KEY_UP) {
                    is_dragging = false;
                    guiReleaseMouseCapture(this);
                }
            } else {
                e.invoke_next();
            }
        });

        subscribe<GuiEvt_MouseMove>([this](const GuiEvt_MouseMove& e) {
            if (guiHasMouseCapture(this)) {
                gfxm::vec2 new_mouse_pos = gfxm::vec2(e.x, e.y);
                if(!is_dragging) {
                    float diff = fabsf(new_mouse_pos.x - mouse_pos.x);
                    if (diff > 10.f) {
                        is_dragging = true;
                        mouse_pos = guiGetMousePos();
                    }
                } else {
                    gfxm::vec2 new_mouse_pos = gfxm::vec2(e.x, e.y);
                    setValue(value + .01f * (new_mouse_pos.x - mouse_pos.x));
                    mouse_pos = new_mouse_pos;

                    if (getParent()) {
                        getParent()->sendMessage(GUI_MSG::NUMERIC_UPDATE, GUI_MSG_PARAMS());
                    }
                }
            }
        });
        
        subscribe<GuiEvt_Unichar>([this](const GuiEvt_Unichar& e) {
            switch (e.ch) {
            case uint32_t(GUI_CHAR::RETURN): {
                is_editing = false;
                updateFromView();
                guiUnfocusWindow(this);
                return;
            }
            }
            e.invoke_next();
        });
    }

    void updateView() {
        setContent(std::format("{:.{}f}", value, decimal_places));
        is_value_dirty = false;
    }
    void updateFromView() {
        std::string str = getText();
        LOG_DBG("updateFromView(): " << str);
        size_t idx = 0;
        if(!str.empty()) {
            float v = .0f;
            auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), v);
            if (ec == std::errc{}) {
                value = v;
            }
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
    
    void layout_2(const gui_layout_context& ctx) override {
        if (is_value_dirty) {
            updateView();
        }
        GuiTextElement::layout_2(ctx);
    }
};

