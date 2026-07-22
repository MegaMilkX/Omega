#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/file_explorer.hpp"
#include "gui/gui_system.hpp"


class GuiResourceRef : public GuiElement {
    GuiTextElement* box = nullptr;
    GuiFileExplorer* browser = nullptr;
public:
    GuiResourceRef(
        const std::string& caption = "ResourceRef"
    ) {
        setSize(gui::fill(), gui::content());
        setStyleClasses({ "control", "container" });
        primary_axis = GUI_PRIMARY_AXIS::X;

        GuiTextElement* label = pushBack(guiCreate<GuiTextElement>(caption));
        label->setReadOnly(true);
        label->setSize(gui::perc(25), gui::em(1.70));
        label->setStyleClasses({ "label" });

        box = pushBack(guiCreate<GuiTextElement>());
        box->setReadOnly(true);
        box->setSize(gui::fill(), gui::em(1.70));
        box->setStyleClasses({ "input-box" });
        box->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
            if(!browser) {
                guiAddTransientScope(box, GUI_TRANSIENT_SCOPE_POP);
                GuiFileExplorerParams params {};
                params.mode = GuiFileExplorerModeOpen;
                // TODO: filter from resource type
                browser = guiCreate<GuiFileExplorer>(params);
                browser->setOwner(box);
                guiGetRoot()->addChild(browser);
                browser->subscribe<GuiEvt_FileConfirmed>([this](const GuiEvt_FileConfirmed& e) {
                    // TODO: Actually update ResourceRefBase
                    // TODO: relative resource-id, not absolute path
                    LOG_DBG("Resource picked: " << e.files[0]);
                    box->setContent(e.files[0]);
                    guiGetRoot()->removeChild(browser);
                    browser = nullptr;
                    guiRemoveTransientScope(box);
                });
            }
        });
        box->subscribe<GuiEvt_ScopeLeft>([this](const GuiEvt_ScopeLeft& e) {
            LOG_DBG("LEFT SCOPE");
            if(browser) {
                guiGetRoot()->removeChild(browser);
                browser = nullptr;
            }
        });
    }

    void setValue(const std::string& val) {
        box->setContent(val);
    }
};

