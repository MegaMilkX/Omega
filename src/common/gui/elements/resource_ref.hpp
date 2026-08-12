#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/image.hpp"
#include "gui/elements/file_explorer.hpp"
#include "gui/gui_system.hpp"


class GuiResourceRef : public GuiElement {
    GuiImage* preview = nullptr;
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

        preview = pushBack(guiCreate<GuiImage>());
        preview->setSize(gui::em(4.f), gui::em(4.f));

        box = pushBack(guiCreate<GuiTextElement>());
        box->setReadOnly(true);
        box->setSize(gui::fill(), gui::em(4));
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

                    std::filesystem::path path(e.files[0]);
                    // TODO: Use explicit resource root instead of current_path
                    path = std::filesystem::relative(path, std::filesystem::current_path());
                    path.replace_extension("");
                    std::string resid = path.generic_string();

                    LOG_DBG("Resource picked: " << resid);
                    guiGetRoot()->removeChild(browser);
                    browser = nullptr;
                    guiRemoveTransientScope(box);

                    invoke(GuiEvt_ResourcePicked(resid));
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

    // TODO: Should not be a texture, but ok for now
    void setPreview(gpuTexture2d* tex) {
        preview->setTexture(tex);
    }

    void setValue(const std::string& val) {
        box->setContent(val);
    }
};

