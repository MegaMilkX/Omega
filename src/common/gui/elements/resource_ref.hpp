#pragma once

#include "gui/elements/element.hpp"
#include "gui/elements/image.hpp"
#include "gui/elements/file_explorer.hpp"
#include "gui/gui_system.hpp"
#include "resource_manager/resource_ref.hpp"


class GuiInspector;
class GuiResourceRef : public GuiElement {
    std::string caption;
    rtti::type resource_type;
    ResourceRefBase* pref = nullptr;

    GuiImage* preview = nullptr;
    GuiTextElement* box = nullptr;
    GuiTextElement* btn_create = nullptr;
    GuiTextElement* btn_expand = nullptr;
    GuiFileExplorer* browser = nullptr;
    GuiMenuList* create_menu = nullptr;
    GuiInspector* inspect_box = nullptr;

    rtti::PropSnapshot snap;
    rtti::PropSnapshot snap_delta;

    void removeCreateMenu() {
        if(create_menu) {
            guiGetRoot()->getPopupLayer()->removeChild(create_menu);
            create_menu = nullptr;
        }
        guiRemoveTransientScope(btn_create);
    }
    void buildCreateMenuImpl(GuiMenuList* list, rtti::type t) {
        auto type_desc = t.get_desc();

        if(t.is_constructible()) {
            list->addItem(t.get_name(), 0)
                ->subscribe<GuiEvt_LClick>([this, t](const GuiEvt_LClick& e) {
                    removeCreateMenu();
                    invoke(GuiEvt_ResourceCreate{ t });
                });
        }

        for (auto derived : type_desc->derived_types) {
            buildCreateMenuImpl(list, derived);
        }
    }
    void initControls();
public:
    GuiResourceRef(ResourceRefBase* pref, const std::string& caption = "ResourceRef");

    void init(ResourceRefBase* pref);

    // TODO: Should not be a texture, but ok for now
    void setPreview(ResourceRef<gpuTexture2d> tex) {
        preview->setTexture(tex);
    }

    void setValue(const std::string& val) {
        box->setContent(val);
    }
};

