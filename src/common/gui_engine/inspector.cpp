#include "inspector.hpp"

#include "resource_manager/resource_ref.hpp"
#include "gui/elements/resource_ref.hpp"
#include "gui/elements/input_numeric.hpp"
#include "gui/elements/checkbox.hpp"
#include "gui/elements/combo_box.hpp"


static rtti::varying* resolveVar(rtti::varying& original, rtti::varying* changed) {
    if (!changed) {
        return &original;
    }
    if (changed->get_type() != original.get_type()) {
        LOG_ERR("resolveVar: original and changed varying TYPE MISMATCH");
        assert(false);
        return &original;
    }
    return changed;
}
static void applySingleChange(rtti::MetaObject* object, const std::string& prop_name, const rtti::varying& var) {
    rtti::PropSnapshot snap;
    snap.add(prop_name, var); // group doesn't matter, they are only for ui
    object->applySnapshot(snap);
}

static void makePropUi(
    GuiElement* container,
    rtti::MetaObject* object,
    rtti::PropSnapshot& snap, // Base object state also serving as the schema
    rtti::PropSnapshot& snap_delta, // Changes accumulated over time in terms of properties touched. The values are not deltas in any way
    const std::string& prop_name,
    rtti::varying& var
) {
    auto prop_type = var.get_type();
    rtti::varying* dvar = snap_delta.get_var(prop_name); // do not capture this, it's not stable
    rtti::varying* pvar = resolveVar(var, dvar);

    if (prop_type.is_enum()) {
        auto gui_combo = guiCreate<GuiComboBox>(prop_name.c_str());
        container->pushBack(gui_combo);
        for (int i = 0; i < prop_type.enumerator_count(); ++i) {
            auto enumerator = prop_type.get_enumerator(i);
            gui_combo->addItem(enumerator->name, enumerator->value);
        }

        gui_combo->setCurrent(pvar->get_enum());
        if (pvar != dvar) {
            gui_combo->addStyleClass("unchanged");
        }
        
        gui_combo->subscribe<GuiEvt_Changed>([object, &snap_delta, prop_type, gui_combo, prop_name](const GuiEvt_Changed& e) {
            gui_combo->removeStyleClass("unchanged");

            rtti::varying v = rtti::varying::make(prop_type);
            v.set_enum(gui_combo->getCurrent());

            // Update the delta snap
            snap_delta.add(prop_name, v);

            // Update the runtime object
            applySingleChange(object, prop_name, v);
        });
    } else if (prop_type.is_wrapper()) {
        if (ResourceRefBase* ref = prop_type.as_resource_ref_base(const_cast<void*>(pvar->data()))) {
            GuiResourceRef* gui_ref = container->pushBack(guiCreate<GuiResourceRef>(ref, prop_name));

            gui_ref->init(ref);
            /*
            gui_ref->setValue(ref->hasEntry() ? ref->getResourceId() : "<NULL>");
            if (prop_type.get_wrapped_type() == rtti::type_get<gpuTexture2d>()) {
                gui_ref->setPreview(*dynamic_cast<ResourceRef<gpuTexture2d>*>(ref));
            }*/
            if (pvar != dvar) {
                gui_ref->addStyleClass("unchanged");
            }

            gui_ref->subscribe<GuiEvt_ResourcePicked>([object, &snap_delta, prop_type, gui_ref, prop_name](const GuiEvt_ResourcePicked& e) {
                LOG_DBG("res_id: " << e.resid);
                gui_ref->removeStyleClass("unchanged");

                rtti::varying v = rtti::varying::make(prop_type);
                ResourceRefBase* ref = prop_type.as_resource_ref_base(const_cast<void*>(v.data()));
                ref->replace(e.resid);

                snap_delta.add(prop_name, v);
                applySingleChange(object, prop_name, v);

                rtti::varying* dvar = snap_delta.get_var(prop_name);
                ResourceRefBase* ref_new = prop_type.as_resource_ref_base(const_cast<void*>(dvar->data()));
                gui_ref->init(ref_new);
            });
            gui_ref->subscribe<GuiEvt_ResourceCreate>([object, &snap_delta, prop_type, gui_ref, prop_name](const GuiEvt_ResourceCreate& e) {
                rtti::varying v = rtti::varying::make(prop_type);
                ResourceRefBase* ref = prop_type.as_resource_ref_base(const_cast<void*>(v.data()));
                
                if (e.type == rtti::type(0)) {
                    ref->reset();
                } else {
                    ref->replaceCreate(e.type);
                    if (!ref->hasEntry()) {
                        assert(false);
                        return;
                    }
                }

                snap_delta.add(prop_name, v);
                applySingleChange(object, prop_name, v);

                rtti::varying* dvar = snap_delta.get_var(prop_name);
                ResourceRefBase* ref_new = prop_type.as_resource_ref_base(const_cast<void*>(dvar->data()));
                gui_ref->init(ref_new);
            });
        } else if(auto wrapped_type = prop_type.get_wrapped_type()) {
            if (wrapped_type.is_derived_from(rtti::type_get<rtti::MetaObject>())) {
                container->pushBack(std::format("[MetaObject] {}: {}", prop_name, var.get_type().get_name()));
            } else {
                container->pushBack(std::format("[Wrapped] {}: {}", prop_name, var.get_type().get_name()));
            }
        }
    } else if (prop_type == rtti::type_get<bool>()) {
        auto gui_input = guiCreate<GuiCheckbox>(prop_name);
        container->pushBack(gui_input);
        gui_input->setValue(*pvar->get<bool>());
        if (pvar != dvar) {
            gui_input->addStyleClass("unchanged");
        }
        gui_input->subscribe<GuiEvt_Changed>([object, &snap_delta, gui_input, prop_name](const GuiEvt_Changed&) {
            gui_input->removeStyleClass("unchanged");
            rtti::varying v = rtti::varying::make<bool>(gui_input->getValue());
            snap_delta.add(prop_name, v);
            applySingleChange(object, prop_name, v);
        });
    } else if (prop_type == rtti::type_get<int>()) {
        auto gui_input = guiCreate<GuiInputNumeric>(prop_name.c_str(), 0);
        container->pushBack(gui_input);
        gui_input->setValue(*pvar->get<int>());
        if (pvar != dvar) {
            gui_input->addStyleClass("unchanged");
        }
        gui_input->on_change = [object, &snap_delta, gui_input, prop_name](int value) {
            gui_input->removeStyleClass("unchanged");
            rtti::varying v = rtti::varying::make<int>(value);
            snap_delta.add(prop_name, v);
            applySingleChange(object, prop_name, v);
        };
    } else if (prop_type == rtti::type_get<float>()) {
        auto gui_input = guiCreate<GuiInputNumeric>(prop_name.c_str());
        container->pushBack(gui_input);
        gui_input->setValue(*pvar->get<float>());
        if (pvar != dvar) {
            gui_input->addStyleClass("unchanged");
        }
        gui_input->on_change = [object, &snap_delta, gui_input, prop_name](float value) {
            gui_input->removeStyleClass("unchanged");
            rtti::varying v = rtti::varying::make<float>(value);
            snap_delta.add(prop_name, v);
            applySingleChange(object, prop_name, v);
        };
    } else if (prop_type == rtti::type_get<gfxm::vec2>()) {
        auto gui_input = guiCreate<GuiInputNumeric2>(prop_name.c_str());
        container->pushBack(gui_input);
        gfxm::vec2 v2 = *pvar->get<gfxm::vec2>();
        gui_input->setValue(v2.x, v2.y);
        if (pvar != dvar) {
            gui_input->addStyleClass("unchanged");
        }
        gui_input->on_change = [object, &snap_delta, gui_input, prop_name](float x, float y) {
            gui_input->removeStyleClass("unchanged");
            rtti::varying v = rtti::varying::make(gfxm::vec2(x, y));
            snap_delta.add(prop_name, v);
            applySingleChange(object, prop_name, v);
        };
    } else if (prop_type == rtti::type_get<gfxm::vec3>()) {
        auto gui_input = guiCreate<GuiInputNumeric3>(prop_name.c_str());
        container->pushBack(gui_input);
        gfxm::vec3 v3 = *pvar->get<gfxm::vec3>();
        gui_input->setValue(v3.x, v3.y, v3.z);
        if (pvar != dvar) {
            gui_input->addStyleClass("unchanged");
        }
        gui_input->on_change = [object, &snap_delta, gui_input, prop_name](float x, float y, float z) {
            gui_input->removeStyleClass("unchanged");
            rtti::varying v = rtti::varying::make(gfxm::vec3(x, y, z));
            snap_delta.add(prop_name, v);
            applySingleChange(object, prop_name, v);
        };
    } else if (prop_type == rtti::type_get<gfxm::vec4>()) {
        auto gui_input = guiCreate<GuiInputNumeric4>(prop_name.c_str());
        container->pushBack(gui_input);
        gfxm::vec4 v4 = *pvar->get<gfxm::vec4>();
        gui_input->setValue(v4.x, v4.y, v4.z, v4.w);
        if (pvar != dvar) {
            gui_input->addStyleClass("unchanged");
        }
        gui_input->on_change = [object, &snap_delta, gui_input, prop_name](float x, float y, float z, float w) {
            gui_input->removeStyleClass("unchanged");
            rtti::varying v = rtti::varying::make(gfxm::vec4(x, y, z, w));
            snap_delta.add(prop_name, v);
            applySingleChange(object, prop_name, v);
        };
    } else if (prop_type == rtti::type_get<gfxm::quat>()) {
        auto gui_input = guiCreate<GuiInputNumeric4>(prop_name.c_str());
        container->pushBack(gui_input);
        gfxm::quat q = *pvar->get<gfxm::quat>();
        gui_input->setValue(q.x, q.y, q.z, q.w);
        if (pvar != dvar) {
            gui_input->addStyleClass("unchanged");
        }
        gui_input->on_change = [object, &snap_delta, gui_input, prop_name](float x, float y, float z, float w) {
            gui_input->removeStyleClass("unchanged");
            rtti::varying v = rtti::varying::make(gfxm::quat(x, y, z, w));
            snap_delta.add(prop_name, v);
            applySingleChange(object, prop_name, v);
        };
    } else if (prop_type == rtti::type_get<std::string>()) {
        auto gui_input = guiCreate<GuiInputString>(prop_name.c_str());
        container->pushBack(gui_input);
        std::string str = *pvar->get<std::string>();
        gui_input->setValue(str);
        if (pvar != dvar) {
            gui_input->addStyleClass("unchanged");
        }
        gui_input->on_change = [object, &snap_delta, gui_input, prop_name](const std::string& str) {
            gui_input->removeStyleClass("unchanged");
            rtti::varying v = rtti::varying::make(str);
            snap_delta.add(prop_name, v);
            applySingleChange(object, prop_name, v);
        };
    } else {
        container->pushBack(new GuiTextElement(std::format("[NO GUI] {}: '{}'", prop_name, var.get_type().get_name()).c_str()));
    }
}


GuiInspector::GuiInspector() {
    setSize(gui::px(400), gui::px(600));
    search_bar = guiCreate<GuiInputString>("Filter");
    pushBack(search_bar);
    search_bar->setSize(gui::fill(), gui::content());
    search_bar->subscribe<GuiEvt_Changed>([this](const GuiEvt_Changed&) {
        filter = search_bar->getValue();
        updateView();
    });

    container = guiCreate<GuiElement>();
    pushBack(container);
    container->setSize(gui::fill(), gui::content());
    container->setStyleClasses({ "window" });
}

void GuiInspector::init(rtti::MetaObject* obj, rtti::PropSnapshot* snap, rtti::PropSnapshot* snap_delta) {
    object = obj;
    this->snap = snap;
    this->snap_delta = snap_delta;
    updateView();
}

void GuiInspector::updateView() {
    GuiElement* gui_elem = container;
    gui_elem->clearChildren();

    for (const auto& group : snap->group_order) {
        auto group_cap = gui_elem->pushBack(group);
        group_cap->setSize(gui::fill(), gui::em(1.70));
        group_cap->setStyleClasses({ "inspector-group-caption" });

        for (const auto& prop_name : snap->group_members[group]) {
            if (prop_name.find(filter) == std::string::npos) {
                continue;
            }
            rtti::varying& var = snap->props[prop_name];
                
            makePropUi(gui_elem, object, *snap, *snap_delta, prop_name, var);
        }
    }
}

