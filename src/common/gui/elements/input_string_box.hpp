#pragma once

#include <functional>
#include "gui/elements/element.hpp"
#include "gui/elements/input_numeric_box.hpp"
#include "gui/elements/text_element.hpp"
#include "gui/gui_system.hpp"


class GuiInputStringBox : public GuiTextElement {
    std::string value;
    bool is_value_dirty = true;

    void setValueDirty() {
        is_value_dirty = true;
    }

public:
    GuiInputStringBox() {
        setReadOnly(false);
        setSize(gui::fill(), gui::em(2));

        setStyleClasses({ "input-box", "input-box-editable", "input-box-string" });

        subscribe<GuiEvt_Unfocus>([this](const GuiEvt_Unfocus& e) {
            updateFromView();
            e.invoke_next();
        });

        subscribe<GuiEvt_Unichar>([this](const GuiEvt_Unichar& e) {            
            switch (e.ch) {
            case uint32_t(GUI_CHAR::RETURN): {
                updateFromView();
                guiUnfocusWindow(this);
                return;
            }
            }
            e.invoke_next();
        });
    }

    void updateView() {
        setContent(value);
        is_value_dirty = false;
    }
    void updateFromView() {
        value = getText();
        setValueDirty();

        if (getParent()) {
            getParent()->sendMessage(GUI_MSG::NUMERIC_UPDATE, GUI_MSG_PARAMS());
        }
    }

    void setValue(const std::string& value) {
        this->value = value;
        setValueDirty();
    }
    const std::string& getValue() const {
        return value;
    }

    void layout_2(const gui_layout_context& ctx) override {
        if (is_value_dirty) {
            updateView();
        }
        GuiTextElement::layout_2(ctx);
    }
};
