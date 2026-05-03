#pragma once

#include <functional>
#include "gui/elements/element.hpp"
#include "gui/elements/input_numeric_box.hpp"
#include "gui/elements/input.hpp"
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
        setSize(gui::fill(), gui::em(2));

        setStyleClasses({ "input-box", "input-box-editable" });

        inner_alignment = HORIZONTAL_ALIGNMENT::LEFT;

        auto unfocus_handler = getHandler<GuiEvt_Unfocus>();
        subscribe<GuiEvt_Unfocus>([this, unfocus_handler](const GuiEvt_Unfocus& e) {
            updateFromView();
            unfocus_handler.invoke(e);
        });

        auto unichar_handler = getHandler<GuiEvt_Unichar>();
        subscribe<GuiEvt_Unichar>([this, unichar_handler](const GuiEvt_Unichar& e) {            
            switch (e.ch) {
            case uint32_t(GUI_CHAR::RETURN): {
                updateFromView();
                guiUnfocusWindow(this);
                return;
            }
            }
            unichar_handler.invoke(e);
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

    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        if (is_value_dirty) {
            updateView();
        }
        GuiTextElement::onLayout(extents, flags);
    }
};
