#pragma once

#include <functional>
#include "gui/elements/element.hpp"
#include "gui/elements/input_numeric_box.hpp"
#include "gui/elements/text_element.hpp"
#include "gui/gui_system.hpp"


class GuiInputStringBox : public GuiTextElement {
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

    void updateFromView() {
        // TODO: eh?
        if (getParent()) {
            getParent()->sendMessage(GUI_MSG::NUMERIC_UPDATE, GUI_MSG_PARAMS());
        }
    }

    void setValue(const std::string& value) {
        setContent(value);
    }
    const std::string& getValue() const {
        return getText();
    }

    void layout_2(const gui_layout_context& ctx) override {
        GuiTextElement::layout_2(ctx);
    }
};
