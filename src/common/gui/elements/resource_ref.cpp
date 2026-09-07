#include "resource_ref.hpp"

#include "gui_engine/inspector.hpp"


void GuiResourceRef::initControls() {
    clearChildren();

    GuiElement* head = guiCreate<GuiElement>();
    head->setSize(gui::fill(), gui::em(4));
    head->primary_axis = GUI_PRIMARY_AXIS::X;
    head->setStyleClasses({ "control", "container" });
    pushBack(head);

    GuiTextElement* label = head->pushBack(guiCreate<GuiTextElement>(caption));
    label->setReadOnly(true);
    label->setSize(gui::perc(25), gui::em(1.70));
    label->setStyleClasses({ "label" });

    preview = head->pushBack(guiCreate<GuiImage>());
    preview->setSize(gui::em(4.f), gui::em(4.f));

    box = head->pushBack(guiCreate<GuiTextElement>());
    box->setReadOnly(true);
    box->setSize(gui::fill(), gui::em(4));
    box->setStyleClasses({ "input-box" });
    box->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
        if(!browser) {
            guiAddTransientScope(box, nullptr, GUI_TRANSIENT_SCOPE_POP);
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

    auto buttons = head->pushBack(guiCreate<GuiElement>());
    buttons->setSize(gui::content(), gui::fill());
    buttons->primary_axis = GUI_PRIMARY_AXIS::Y;
    buttons->setStyleClasses({ "control", "container" });


    btn_create = buttons->pushBack(guiCreate<GuiTextElement>());
    btn_create->setReadOnly(true);
    btn_create->setSize(gui::content(), gui::fill());
    btn_create->setStyleClasses({ "button", "icon" });
    btn_create->setContent(ICON_FK_PLUS);
    btn_create->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
        if (!create_menu) {
            guiAddTransientScope(btn_create, nullptr, GUI_TRANSIENT_SCOPE_POP);
            create_menu = guiCreate<GuiMenuList>();
            create_menu->setOwner(btn_create);
            guiGetRoot()->getPopupLayer()->addChild(create_menu);

            gfxm::vec2 pos = guiConvertToLocal(guiGetRoot()->getPopupLayer(), guiGetMousePos());
            create_menu->setPosition(gui_vec2(pos.x, pos.y));
                
            create_menu->addItem("<NULL>", 0)
                ->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
                    removeCreateMenu();
                    invoke(GuiEvt_ResourceCreate{ rtti::type(0) });
                });
            buildCreateMenuImpl(create_menu, resource_type);
        }
    });
    btn_create->subscribe<GuiEvt_ScopeLeft>([this](const GuiEvt_ScopeLeft& e) {
        if(create_menu) {
            guiGetRoot()->getPopupLayer()->removeChild(create_menu);
            create_menu = nullptr;
        }
    });
    

    if (resource_type.is_derived_from(rtti::type_get<rtti::MetaObject>())) {
        btn_expand = buttons->pushBack(guiCreate<GuiTextElement>());
        btn_expand->setReadOnly(true);
        btn_expand->setSize(gui::content(), gui::fill());
        btn_expand->setStyleClasses({ "button", "icon" });
        btn_expand->setContent(ICON_FK_CHEVRON_DOWN);
        btn_expand->subscribe<GuiEvt_LClick>([this](const GuiEvt_LClick& e) {
            if (inspect_box) {
                inspect_box->remove();
                inspect_box = nullptr;
            } else {
                auto ref = this->pref;

                if (!ref->hasEntry()) {
                    return;
                }

                if (!resource_type.is_derived_from(rtti::type_get<rtti::MetaObject>())) {
                    return;
                }

                ResourceEntry* entry = ref->getEntry();
                void* object = entry->data;

                // TODO: PROPER CAST
                auto mo = static_cast<rtti::MetaObject*>(object);
                mo->makeSnapshot(snap);

                inspect_box = guiCreate<GuiInspector>();
                inspect_box->setSize(gui::fill(), gui::content());
                inspect_box->addStyleClass("embedded-inspector");
                pushBack(inspect_box);
                inspect_box->init(mo, &snap, &snap_delta);
            }
        });
    }


    // ============================================
    if (pref->hasEntry()) {
        if (pref->getResourceId().empty()) {
            setValue("[EMBEDDED]");
        } else {
            setValue(pref->getResourceId());
        }
    } else {
        setValue("<NULL>");
    }

    if (resource_type == rtti::type_get<gpuTexture2d>()) {
        setPreview(*dynamic_cast<ResourceRef<gpuTexture2d>*>(pref));
    }
}

GuiResourceRef::GuiResourceRef(ResourceRefBase* pref, const std::string& caption)
: caption(caption), pref(pref) {
    resource_type = pref->getRefType();

    setSize(gui::fill(), gui::content());
    setStyleClasses({ "control", "container" });
    primary_axis = GUI_PRIMARY_AXIS::Y;

    initControls();
}

void GuiResourceRef::init(ResourceRefBase* pref) {
    this->pref = pref;
    resource_type = pref->getRefType();

    initControls();
}