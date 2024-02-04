#pragma once

#include "editor_window.hpp"
#include "gui/build.hpp"
#include "gui/elements/list.hpp"


class GuiSelectListWindow : public GuiWindow {
    GuiElement* container = 0;
    GuiListItem* selected = 0;
public:
    std::function<void(int)> on_chosen;
    std::function<void(void)> on_cancelled;

    GuiSelectListWindow()
    : GuiWindow("Select anim graph unit") {
        auto mpos = guiGetMousePos();
        setPosition(mpos.x - 100, mpos.y - 50);
        setSize(200, 250);
        addFlags(GUI_FLAG_BLOCKING);
        addFlags(GUI_FLAG_TOPMOST);
        overflow = GUI_OVERFLOW_FIT;

        container = pushBack(new GuiElement());
        container->setStyleClasses({ "list" });
        container->setSize(0, 170);
        auto btn_choose = pushBack(new GuiButton("Choose"));
        btn_choose->on_click = [this]() {
            if (!selected) {
                return;
            }
            if (on_chosen) {
                on_chosen(selected->item_id);
            }
            sendMessage(GUI_MSG::CLOSE, 0, 0);
        };
        auto btn_cancel = pushBack(new GuiButton("Cancel"));
        btn_cancel->setFlags(GUI_FLAG_SAME_LINE);
        btn_cancel->on_click = [this]() {
            if (on_cancelled) {
                on_cancelled();
            }
            sendMessage(GUI_MSG::CLOSE, 0, 0);
        };
    }

    void addItem(const char* caption, int id) {
        auto itm = new GuiListItem(caption, id);
        itm->on_click = [this, itm]() {
            if (selected) {
                selected->setSelected(false);
            }
            selected = itm;
            itm->setSelected(true);
        };
        itm->on_double_click = [this, itm]() {
            if (on_chosen) {
                on_chosen(selected->item_id);
            }
            sendMessage(GUI_MSG::CLOSE, 0, 0);
        };
        container->pushBack(itm);
    }
};


class GuiAnimatorDocument : public GuiEditorWindow {
    //GuiElement* gui_tools_general = 0;
    //GuiElement* gui_tools_anim_list = 0;
    //GuiElement* gui_tools_properties = 0;

    RHSHARED<mdlSkeletalModelMaster> reference_model;
    GuiList* unit_list = 0;
public:
    GuiAnimatorDocument()
        : GuiEditorWindow("Animator", "animator") {
        auto dock_space = pushBack(new GuiDockSpace(this));
        
        GuiWindow* wnd_tools = 0;
        GuiWindow* wnd_canvas = 0;

        {
            wnd_tools = guiGetRoot()->pushBack(new GuiWindow("Tools"));
            auto cont = wnd_tools->pushBack(new GuiElement());
            cont->pushBack(new GuiLabel("General"));
            cont->pushBack(new GuiInputFilePath(
                "Reference",
                std::bind(&GuiAnimatorDocument::setReferenceModel, this, std::placeholders::_1),
                std::bind(&GuiAnimatorDocument::getReferenceModelPath, this),
                GUI_INPUT_FILE_READ, "skeletal_model", fsGetCurrentDirectory().c_str()
            ));
            cont->overflow = GUI_OVERFLOW_FIT;

            cont = wnd_tools->pushBack(new GuiElement());
            cont->pushBack(new GuiLabel("Units"));
            unit_list = cont->pushBack(new GuiList());
            unit_list->setOwner(this);
            unit_list->on_add = [this]() {
                auto wnd = new GuiSelectListWindow();
                wnd->addItem("Single clip", 0);
                wnd->addItem("State machine", 1);
                wnd->addItem("Blend tree", 2);
                wnd->on_chosen = [this](int item_id) {
                    unit_list->pushBack(new GuiListItem("Hello", 0));
                };
                wnd->on_cancelled = [this]() {
                    // Idk, nothing probably
                };
                guiAddManagedWindow(guiGetRoot()->pushBack(wnd));
            };
            unit_list->on_remove = []() {
                // TODO:
            };
            cont->overflow = GUI_OVERFLOW_FIT;

            cont = wnd_tools->pushBack(new GuiElement());
            cont->pushBack(new GuiLabel("Anim list"));
            cont->pushBack(new GuiAnimationSyncList());
            cont->overflow = GUI_OVERFLOW_FIT;

            cont = wnd_tools->pushBack(new GuiElement());
            cont->pushBack(new GuiLabel("Properties"));
            cont->pushBack(new GuiAnimationPropList());
            cont->overflow = GUI_OVERFLOW_FIT;

            wnd_canvas = guiGetRoot()->pushBack(new GuiWindow("Canvas"));
            wnd_canvas->pushBack(new GuiButton("Root"));
        }

        dock_space->getRoot()->splitX("Canvas", "Tools", 0.7f);
        dock_space->getRoot()->left->setLocked(true);
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
