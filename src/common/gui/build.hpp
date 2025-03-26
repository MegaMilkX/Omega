#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/window.hpp"
#include "gui/elements/anim_list.hpp"
#include "gui/elements/anim_prop_list.hpp"
#include "gui/gui.hpp"


namespace gui {
namespace build {

    // Returns false or breaks in debug mode if there are any unterminated begin*** blocks
    bool ValidateBuilderState();

    // Styling functions

    void Position(float x, float y);
    void Size(float x, float y);
    void ContentPadding(float left, float top, float right, float bottom);
    void Flags(gui_flag_t flags);

    // Building functions

    GuiWindow* BeginWindow(const char* title);
    void EndWindow();

    GuiElement* Begin();
    void End();

    GuiLabel* Label(const char* caption);
    GuiButton* Button(const char* caption);
    GuiInputFilePath* InputFile(
        const char* caption = "InputFilePath",
        std::string* output = 0,
        GUI_INPUT_FILE_TYPE type = GUI_INPUT_FILE_READ,
        const char* filter = "",
        const char* root_dir = ""
    );
    GuiInputFilePath* InputFile(
        const char* caption,
        std::function<void(const std::string&)> set_cb,
        std::function<std::string(void)> get_cb,
        GUI_INPUT_FILE_TYPE type = GUI_INPUT_FILE_READ,
        const char* filter = "",
        const char* root_dir = ""
    );


    GuiAnimationSyncList* AnimationSyncList();
    GuiAnimationPropList* AnimationPropList();

    

}
}