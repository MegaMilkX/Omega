#include "inspector.hpp"

#include "resource_manager/resource_ref.hpp"
#include "gui/elements/resource_ref.hpp"
#include "gui/elements/input_numeric.hpp"
#include "gui/elements/checkbox.hpp"
#include "gui/elements/combo_box.hpp"


static void makePropUi(
    GuiElement* container,
    rtti::MetaObject* object,
    rtti::PropSnapshot& snap,
    const std::string& prop_name,
    rtti::varying& var
) {
    auto prop_type = var.get_type();

    if (prop_type.is_enum()) {
        auto gui_input = guiCreate<GuiComboBox>(prop_name.c_str());
        container->pushBack(gui_input);
        for (int i = 0; i < prop_type.enumerator_count(); ++i) {
            auto enumerator = prop_type.get_enumerator(i);
            gui_input->addItem(enumerator->name, enumerator->value);
        }
        gui_input->setCurrent(var.get_enum());
        gui_input->subscribe<GuiEvt_Changed>([object, &snap, &var, gui_input](const GuiEvt_Changed& e) {
            var.set_enum(gui_input->getCurrent());
            object->applySnapshot(snap);
        });
    } else if (prop_type.is_wrapper()) {
        if (ResourceRefBase* ref = prop_type.as_resource_ref_base(const_cast<void*>(var.data()))) {
            GuiResourceRef* gui_ref = container->pushBack(guiCreate<GuiResourceRef>(prop_name));
            if(ref->hasEntry()) {
                std::string res_id = ref->getResourceId();
                gui_ref->setValue(res_id);
            } else {
                gui_ref->setValue("<NULL>");
            }
            gui_ref->subscribe<GuiEvt_ResourcePicked>([object, &snap, ref, gui_ref](const GuiEvt_ResourcePicked& e) {
                LOG_DBG("res_id: " << e.resid);
                ref->replace(e.resid);
                if (ref->hasEntry()) {
                    gui_ref->setValue(e.resid);
                } else {
                    gui_ref->setValue("<NULL>");
                }
                object->applySnapshot(snap);
            });
        } else {
            auto wrapped_type = prop_type.get_wrapped_type();
            if (wrapped_type.is_derived_from(rtti::type_get<rtti::MetaObject>())) {
                container->pushBack(std::format("[MetaObject] {}: {}", prop_name, var.get_type().get_name()));
            } else {
                container->pushBack(std::format("[Wrapped] {}: {}", prop_name, var.get_type().get_name()));
            }
        }
    } else if (prop_type == rtti::type_get<bool>()) {
        auto gui_input = guiCreate<GuiCheckbox>(prop_name);
        container->pushBack(gui_input);
        gui_input->setValue(*var.get<bool>());
        gui_input->subscribe<GuiEvt_Changed>([object, &snap, &var, gui_input](const GuiEvt_Changed&) {
            var.set<bool>(gui_input->getValue());
            object->applySnapshot(snap);
        });
    } else if (prop_type == rtti::type_get<int>()) {
        auto gui_input = guiCreate<GuiInputNumeric>(prop_name.c_str(), 0);
        container->pushBack(gui_input);
        gui_input->setValue(*var.get<int>());
        gui_input->on_change = [object, &snap, &var](int value) {
            var.set<int>(value);
            object->applySnapshot(snap);
        };
    } else if (prop_type == rtti::type_get<float>()) {
        auto gui_input = guiCreate<GuiInputNumeric>(prop_name.c_str());
        container->pushBack(gui_input);
        gui_input->setValue(*var.get<float>());
        gui_input->on_change = [var](float value) {
            //var = std::move(rtti::varying::make(value));
        };
    } else if (prop_type == rtti::type_get<gfxm::vec2>()) {
        auto gui_input = guiCreate<GuiInputNumeric2>(prop_name.c_str());
        container->pushBack(gui_input);
        gfxm::vec2 v2 = *var.get<gfxm::vec2>();
        gui_input->setValue(v2.x, v2.y);
        gui_input->on_change = [](float x, float y) {
            // TODO:
        };
    } else if (prop_type == rtti::type_get<gfxm::vec3>()) {
        auto gui_input = guiCreate<GuiInputNumeric3>(prop_name.c_str());
        container->pushBack(gui_input);
        gfxm::vec3 v3 = *var.get<gfxm::vec3>();
        gui_input->setValue(v3.x, v3.y, v3.z);
        gui_input->on_change = [](float x, float y, float z) {
            // TODO:
        };
    } else if (prop_type == rtti::type_get<gfxm::vec4>()) {
        auto gui_input = guiCreate<GuiInputNumeric4>(prop_name.c_str());
        container->pushBack(gui_input);
        gfxm::vec4 v4 = *var.get<gfxm::vec4>();
        gui_input->setValue(v4.x, v4.y, v4.z, v4.w);
        gui_input->on_change = [](float x, float y, float z, float w) {
            // TODO:
        };
    } else if (prop_type == rtti::type_get<gfxm::quat>()) {
        auto gui_input = guiCreate<GuiInputNumeric4>(prop_name.c_str());
        container->pushBack(gui_input);
        gfxm::quat q = *var.get<gfxm::quat>();
        gui_input->setValue(q.x, q.y, q.z, q.w);
        gui_input->on_change = [](float x, float y, float z, float w) {
            // TODO:
        };
    } else if (prop_type == rtti::type_get<std::string>()) {
        auto gui_input = guiCreate<GuiInputString>(prop_name.c_str());
        container->pushBack(gui_input);
        std::string str = *var.get<std::string>();
        gui_input->setValue(str);
        gui_input->on_change = [](const std::string& str) {
            // TODO:
        };
    } else {
        container->pushBack(new GuiTextElement(std::format("[NO GUI] {}: {}", prop_name, var.get_type().get_name()).c_str()));
    }
}


GuiInspector::GuiInspector() {
    setSize(gui::px(400), gui::px(600));
    search_bar = guiCreate<GuiInputString>("Search");
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

void GuiInspector::init(rtti::MetaObject* obj, rtti::PropSnapshot* snap) {
    object = obj;
    this->snap = snap;
    updateView();
}

void GuiInspector::updateView() {
    GuiElement* gui_elem = container;
    gui_elem->clearChildren();

    gui_elem->pushBack(snap->type_.get_name());
    /*
    {
        GuiImage* gui_img = gui_elem->pushBack(guiCreate<GuiImage>(nullptr));
        ResourceRefBase* ref = &test_texture;
        std::string res_id = ref->getResourceId();
        GuiResourceRef* gui_ref = gui_elem->pushBack(guiCreate<GuiResourceRef>("texture"));
        gui_ref->setValue(res_id);
        gui_ref->subscribe<GuiEvt_ResourcePicked>([this, ref, gui_img](const GuiEvt_ResourcePicked& e) {
            LOG_DBG("res_id: " << e.resid);
            ref->replace(e.resid);
            if (ref) {
                gui_img->setTexture(test_texture.get());
            } else {
                gui_img->setTexture(nullptr);
            }
        });
    }*/

    for (const auto& group : snap->group_order) {
        auto group_cap = gui_elem->pushBack(group);
        group_cap->setSize(gui::fill(), gui::em(1.70));
        group_cap->setStyleClasses({ "inspector-group-caption" });

        for (const auto& prop_name : snap->group_members[group]) {
            if (!prop_name.starts_with(filter)) {
                continue;
            }
            rtti::varying& var = snap->props[prop_name];
                
            makePropUi(gui_elem, object, *snap, prop_name, var);
        }
    }
}

