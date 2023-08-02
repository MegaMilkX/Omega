#pragma once

#include "editor_window.hpp"
#include "gui/build.hpp"


class GuiAnimatorDocument : public GuiEditorWindow {
    GuiElement* gui_tools_general = 0;
    GuiElement* gui_tools_anim_list = 0;
    GuiElement* gui_tools_properties = 0;

    RHSHARED<mdlSkeletalModelMaster> reference_model;
public:
    GuiAnimatorDocument()
        : GuiEditorWindow("Animator") {
        padding = gfxm::rect(0, 0, 0, 0);
        auto dock_space = new GuiDockSpace(this);
        guiAdd(this, this, dock_space);
        
        GuiWindow* wnd_tools = 0;
        GuiWindow* wnd_canvas = 0;
        {
            using namespace gui::build;

            ContentPadding(0, 0, 0, 0);
            if (wnd_tools = BeginWindow("Tools")) {
                Overflow(GUI_OVERFLOW_FIT);
                if (gui_tools_general = Begin()) {
                    Flags(GUI_FLAG_PERSISTENT);
                    Label("General");
                    InputFile("Reference", 
                        std::bind(&GuiAnimatorDocument::setReferenceModel, this, std::placeholders::_1),
                        std::bind(&GuiAnimatorDocument::getReferenceModelPath, this),
                        GUI_INPUT_FILE_READ, "skeletal_model", fsGetCurrentDirectory().c_str()
                    );
                    End();
                }

                Overflow(GUI_OVERFLOW_FIT);
                if (gui_tools_anim_list = Begin()) {
                    Label("Anim list");
                    AnimationSyncList();
                    End();
                }

                Overflow(GUI_OVERFLOW_FIT);
                if (gui_tools_properties = Begin()) {
                    Label("Properties");
                    AnimationPropList();
                    End();
                }

                EndWindow();
            }
            if (wnd_canvas = BeginWindow("Canvas")) {
                Flags(GUI_FLAG_FLOATING);
                Position(500, 500);
                Button("Select an Animator root processor...");
                EndWindow();
            }
        }

        dock_space->getRoot()->splitX("Canvas", "Tools", 0.7f);
        dock_space->insert("Tools", wnd_tools);
        dock_space->insert("Canvas", wnd_canvas);
    }

    void setReferenceModel(const std::string& path) {
        reference_model = resGet<mdlSkeletalModelMaster>(path.c_str());
    }
    std::string getReferenceModelPath() {
        return reference_model.getReferenceName();
    }
};