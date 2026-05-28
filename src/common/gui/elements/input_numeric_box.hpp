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
        setReadOnly(false);
        setSize(gui::fill(), gui::em(2));
        setStyleClasses({ "input-box" });

        auto focus_handler = getHandler<GuiEvt_Focus>();
        subscribe<GuiEvt_Focus>([this, focus_handler](const GuiEvt_Focus& e) {
            if (is_editing) {
                e.new_focused = this;
                focus_handler.invoke(e);
            } else {
                e.consume = false;
            }
        });
        auto unfocus_handler = getHandler<GuiEvt_Unfocus>();
        subscribe<GuiEvt_Unfocus>([this, unfocus_handler](const GuiEvt_Unfocus& e) {
            is_editing = false;
            setStyleClasses({ "input-box" });
            updateFromView();
            unfocus_handler.invoke(e);
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

        auto mouse_btn_handler = getHandler<GuiEvt_MouseBtn>();
        subscribe<GuiEvt_MouseBtn>([this, mouse_btn_handler](const GuiEvt_MouseBtn& e) {
            if (e.btn == GUI_MOUSE_LEFT) {
                if (e.state == GUI_KEY_DOWN) {
                    if(!is_editing) {
                        guiCaptureMouse(this);
                        mouse_pos = guiGetMousePos();
                    } else {
                        mouse_btn_handler.invoke(e);
                    }
                } else if (e.state == GUI_KEY_UP) {
                    is_dragging = false;
                    guiReleaseMouseCapture(this);
                }
            } else {
                mouse_btn_handler.invoke(e);
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
        
        auto unichar_handler = getHandler<GuiEvt_Unichar>();
        subscribe<GuiEvt_Unichar>([this, unichar_handler](const GuiEvt_Unichar& e) {
            switch (e.ch) {
            case uint32_t(GUI_CHAR::RETURN): {
                is_editing = false;
                updateFromView();
                guiUnfocusWindow(this);
                return;
            }
            }
            unichar_handler.invoke(e);
        });
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
    
    void onLayout(const gui_layout_context& ctx) override {
        if (is_value_dirty) {
            updateView();
        }
        GuiTextElement::onLayout(ctx);
    }
};

