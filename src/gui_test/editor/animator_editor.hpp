#pragma once

#include "editor_window.hpp"
#include "gui/build.hpp"
#include "gui/elements/list.hpp"
#include "gui/elements/state_graph.hpp"
#include "gui/lib/nativefiledialog/nfd.h"


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
                on_chosen((int)selected->user_ptr);
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
        auto itm = new GuiListItem(caption, (void*)id);
        itm->on_click = [this, itm]() {
            if (selected) {
                selected->setSelected(false);
            }
            selected = itm;
            itm->setSelected(true);
        };
        itm->on_double_click = [this, itm]() {
            if (on_chosen) {
                on_chosen((int)selected->user_ptr);
            }
            sendMessage(GUI_MSG::CLOSE, 0, 0);
        };
        container->pushBack(itm);
    }
};


class GuiAnimUnit : public GuiElement {
    std::string unit_name;
public:
    GuiAnimUnit(const char* unit_name)
        : unit_name(unit_name) {}
    
    void setUnitName(const char* name) { unit_name = name; }
    const std::string& getUnitName() const { return unit_name; }
};

class GuiAnimUnitClip : public GuiAnimUnit {
public:
    GuiAnimUnitClip(const char* name)
    : GuiAnimUnit(name) {}
};
class GuiAnimUnitStateGraph : public GuiAnimUnit {
public:
    GuiAnimUnitStateGraph(const char* name)
    : GuiAnimUnit(name) {
        setSize(0, 0);
        pushBack(new GuiStateGraph);
    }
};
class GuiAnimUnitBlendTree : public GuiAnimUnit {
public:
    GuiAnimUnitBlendTree(const char* name)
    : GuiAnimUnit(name) {}
};

enum ANIM_UNIT {
    ANIM_UNIT_CLIP,
    ANIM_UNIT_STATE_GRAPH,
    ANIM_UNIT_BLEND_TREE
};

class GuiAnimatorDocument : public GuiEditorWindow {
    //GuiElement* gui_tools_general = 0;
    //GuiElement* gui_tools_anim_list = 0;
    //GuiElement* gui_tools_properties = 0;

    GuiWindow* wnd_tools = 0;
    GuiWindow* wnd_canvas = 0;

    GuiComboBox* root_unit_combo = 0;

    RHSHARED<mdlSkeletalModelMaster> reference_model;
    GuiList* unit_list = 0;
    GuiList* anim_list = 0;
    GuiList* param_list = 0;

    std::vector<std::shared_ptr<GuiAnimUnit>> units;
    GuiAnimUnit* root_unit = 0;
    GuiAnimUnit* displayed_unit = 0;

    void setDisplayedUnit(GuiAnimUnit* unit) {
        displayed_unit = unit;
        wnd_canvas->clearChildren();
        if (unit == 0) {
            return;
        }
        wnd_canvas->pushBack(unit);
    }
    void setRootUnit(GuiAnimUnit* unit) {
        root_unit = unit;
        if (displayed_unit == 0) {
            setDisplayedUnit(unit);
        }
    }
    void updateRootSelectList(bool refreshList = true) {
        if (refreshList) {
            root_unit_combo->clearChildren();
            for (int i = 0; i < units.size(); ++i) {
                auto item = new GuiMenuListItem(units[i]->getUnitName().c_str(), 0);
                item->on_click = [this, i]() {
                    setRootUnit(units[i].get());
                    updateRootSelectList(false);
                };
                root_unit_combo->pushBack(item);
            }
        }
        if (root_unit) {
            root_unit_combo->setValue(root_unit->getUnitName().c_str());
        } else {
            root_unit_combo->setValue("<NULL>");
        }
    }
public:
    GuiAnimatorDocument()
        : GuiEditorWindow("Animator", "animator") {
        auto dock_space = pushBack(new GuiDockSpace(this));
        

        {
            wnd_tools = guiGetRoot()->pushBack(new GuiWindow("Tools"));
            auto cont = wnd_tools->pushBack(new GuiElement());
            cont->overflow = GUI_OVERFLOW_FIT;
            cont->pushBack(new GuiLabel("General"));
            cont->pushBack(new GuiInputFilePath(
                "Reference",
                std::bind(&GuiAnimatorDocument::setReferenceModel, this, std::placeholders::_1),
                std::bind(&GuiAnimatorDocument::getReferenceModelPath, this),
                GUI_INPUT_FILE_READ, "skeletal_model", fsGetCurrentDirectory().c_str()
            ));
            root_unit_combo = new GuiComboBox("Root unit");
            updateRootSelectList();
            cont->pushBack(root_unit_combo);

            cont->pushBack(new GuiLabel("Units"));
            unit_list = cont->pushBack(new GuiList());
            unit_list->setOwner(this);
            unit_list->on_add = [this](GuiListItem* item)->bool {
                auto wnd = new GuiSelectListWindow();
                wnd->addItem("Clip", ANIM_UNIT_CLIP);
                wnd->addItem("State Machine", ANIM_UNIT_STATE_GRAPH);
                wnd->addItem("Blend Tree", ANIM_UNIT_BLEND_TREE);
                wnd->on_chosen = [this, item](int item_id) {
                    const char* name = 0;
                    std::shared_ptr<GuiAnimUnit> unit;
                    switch (item_id) {
                    case ANIM_UNIT_CLIP:
                        name = "Clip";
                        unit.reset(new GuiAnimUnitClip(name));
                        break;
                    case ANIM_UNIT_STATE_GRAPH: {
                        name = "State Machine";
                        auto graph = new GuiAnimUnitStateGraph(name);
                        unit.reset(graph);
                        break;
                    }
                    case ANIM_UNIT_BLEND_TREE:
                        name = "Blend Tree";
                        unit.reset(new GuiAnimUnitBlendTree(name));
                        break;
                    default:
                        LOG_ERR("Unknown anim graph unit type: " << item_id);
                        assert(false);
                        return;
                    }
                    GuiAnimUnit* punit = unit.get();
                    item->user_ptr = punit;
                    item->clearChildren();
                    item->pushBack(name);
                    item->on_click = [this, punit]() {
                        setDisplayedUnit(punit);
                    };
                    units.push_back(unit);

                    updateRootSelectList();
                };
                wnd->on_cancelled = [this, item]() {
                    unit_list->removeChild(item);
                };
                guiAddManagedWindow(guiGetRoot()->pushBack(wnd));
                return true;
            };
            unit_list->on_remove = [this](GuiListItem* item) {
                GuiAnimUnit* unit = (GuiAnimUnit*)item->user_ptr;
                unit_list->removeChild(item);
                for (int i = 0; i < units.size(); ++i) {
                    if (units[i].get() == unit) {
                        units.erase(units.begin() + i);
                        break;
                    }
                }
                if (root_unit == unit) {
                    setRootUnit(0);
                }
                if (displayed_unit == unit) {
                    setDisplayedUnit(0);
                }
                updateRootSelectList();
            };

            cont->pushBack(new GuiLabel("Anim list"));
            //cont->pushBack(new GuiAnimationSyncList());
            anim_list = new GuiList(true);
            cont->pushBack(anim_list);
            anim_list->on_add_group = [this](GuiTreeItem* group)->bool {
                group->setCaption("SyncGroup");
                return true;
            };
            anim_list->on_remove_group = [this](GuiTreeItem* group) {
                // TODO:
            };
            anim_list->on_add_to_group = [this](GuiTreeItem* group, GuiListItem* item)->bool {
                nfdchar_t* nfdpath = 0;
                nfdresult_t result = NFD_OpenDialog("anim", 0, &nfdpath);
                if(result == NFD_CANCEL) {
                    return false;
                } else if(result != NFD_OKAY) {
                    LOG_ERR("Browse dialog error: " << result);
                    return false;
                }
                std::string path = nfdpath;
                item->clearChildren();
                item->pushBack(path.c_str());
                item->user_ptr = 0;
                return true;
            };
            anim_list->on_remove_from_group = [this](GuiTreeItem* group, GuiListItem* item) {
                // TODO:
            };
            

            cont->pushBack(new GuiLabel("Properties"));
            //cont->pushBack(new GuiAnimationPropList());
            param_list = new GuiList(false);
            cont->pushBack(param_list);

            wnd_canvas = guiGetRoot()->pushBack(new GuiWindow("Canvas"));
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
