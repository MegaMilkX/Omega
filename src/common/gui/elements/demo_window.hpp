#pragma once

#include "gui/elements/window.hpp"
#include "gui/elements/collapsing_header.hpp"
#include "gui/elements/input_string.hpp"
#include "gui/elements/input_numeric.hpp"
#include "gui/elements/input_resource.hpp"
#include "gui/elements/combo_box.hpp"
#include "gui/elements/tree_view.hpp"
#include "gui/elements/image.hpp"
#include "gui/elements/notification.hpp"


class GuiDemoWindow : public GuiWindow {
public:
    GuiDemoWindow()
    : GuiWindow("DemoWindow") {
        setPosition(850, 200);
        setSize(400, 600);

        pushBack(new GuiNotification("Notification box"));

        auto header_input = pushBack(new GuiCollapsingHeader("Inputs"));

        header_input->pushBack(new GuiInputString)->setValue("Text string");
        header_input->pushBack(new GuiInputNumeric);
        header_input->pushBack(new GuiInputNumeric2);
        header_input->pushBack(new GuiInputNumeric3);
        header_input->pushBack(new GuiInputNumeric4);
        header_input->pushBack(new GuiCheckbox());
        //header_input->pushBack(new GuiResourceRef());
        header_input->pushBack(new GuiComboBox())->addItem("Test", 0);
        header_input->pushBack(new GuiInputResource);

        auto header_old_input = pushBack(new GuiCollapsingHeader("Old Inputs"));

        header_old_input->pushBack(new GuiTextElement("Hello, World!"));
        header_old_input->pushBack(new GuiComboBox());
        header_old_input->pushBack(new GuiCollapsingHeader("CollapsingHeader", true))
            ->pushBack("Hello, World!");
        
        auto header_text = pushBack(new GuiCollapsingHeader("Text"));
        
        auto para = header_text->pushBack(R"(Then Fingolfin beheld (as it seemed to him) the utter ruin of the Noldor,
and the defeat beyond redress of all their houses;
and filled with wrath and despair he mounted upon Rochallor his great horse and rode forth alone,
and none might restrain him.)",
            { "paragraph" }
        );
        para->setSize(gui::fill(), gui::content());
        dynamic_cast<GuiTextElement*>(para)->setReadOnly(false);
        
        GuiElement* head = new GuiElement;
        head->setSize(gui::perc(100), gui::em(7));
        head->setStyleClasses({ "control", "notification" });
        header_text->pushBack(head);

        para = header_text->pushBack(R"(He passed over Dor-nu-Fauglith like a wind amid the dust,
and all that beheld his onset fled in amaze, thinking that Orome himself was come:
for a great madness of rage was upon him, so that his eyes shone like the eyes of the Valar.
Thus he came alone to Angband's gates, and he sounded his horn,
and smote once more upon the brazen doors,
and challenged Morgoth to come forth to single combat. And Morgoth came.)",
            { "paragraph" }
        );
        para->setSize(gui::fill(), gui::content());

        GuiTextElement* text = new GuiTextElement;
        text->setContent("Example notification");
        text->setStyleClasses({ "header" });
        GuiTextElement* text2 = new GuiTextElement;
        text2->setContent("Notification body");
        text2->setStyleClasses({ "paragraph" });
        head->pushBack(text);
        head->pushBack(text2);

        auto header_other = pushBack(new GuiCollapsingHeader("Other"));

        header_other->pushBack(new GuiTreeView(), GUI_FLAG_RESIZE);
        header_other->pushBack(new GuiImage(loadResource<gpuTexture2d>("1648920106773")));
        header_other->pushBack(new GuiButton("Button A"));
        header_other->pushBack(new GuiButton("Button B"));
    }
};

