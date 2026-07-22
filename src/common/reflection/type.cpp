#include "type.hpp"

#include <queue>
#include "type_desc.hpp"
#include "varying.hpp"
#include "log/log.hpp"


namespace rtti {


size_t      type::get_size() const {
    auto desc = get_type_desc(*this);
    return desc->size;
}
const char* type::get_name() const {
    auto desc = get_type_desc(*this);
    return desc->name.c_str();
}
const type_desc* type::get_desc() const {
    auto desc = get_type_desc(*this);
    return desc;
}

bool type::is_valid() const {
    return id != 0;
}
bool type::is_pointer() const {
    auto desc = get_type_desc(*this);
    return desc->is_pointer;
}
bool type::is_wrapper() const {
    auto desc = get_type_desc(*this);
    return desc->is_wrapper;
}
type type::get_wrapped_type() const {
    auto desc = get_type_desc(*this);
    return desc->wrapped_type;
}
bool type::is_copy_constructible() const {
    auto desc = get_type_desc(*this);
    return desc->pfn_copy_construct != nullptr;
}
bool type::is_derived_from(type other) const {
    type current_type = *this;
    std::queue<type> type_q;
    while (current_type) {
        auto desc = get_type_desc(current_type);
        for (auto& parent_info : desc->parent_types) {
            type_q.push(parent_info.parent_type);
        }

        if (current_type == other) {
            return true;
        }

        if (type_q.empty()) {
            current_type = type(0);
        } else {
            current_type = type_q.front();
            type_q.pop();
        }
    }

    return false;
}
int   type::prop_count() const {
    return get_desc()->properties.size();
}
const type_property_desc* type::get_prop(int i) {
    return &get_desc()->properties[i];
}
property type::get_property(int i) {
    return property(id, i);
}

varying type::get_prop_value(const MetaObject* object, int prop_idx) {
    auto prop = get_prop(prop_idx);
    //varying var;
    //var.set(prop->t, prop->fn_get_ptr(object));
    return prop->get_value(object);
}

void type::set_property_unsafe(const char* name, MetaObject* object, void* value) const {
    for (auto& prop : get_desc()->properties) {
        if (prop.name != name) {
            continue;
        }
        if (prop.fn_set) {
            prop.fn_set(object, value);
        }
        // TODO: only handles properties with setters right now (not objects)
    }
}

void type::dbg_print() {
    auto desc = get_type_desc(*this);
    LOG_DBG(desc->name << "(" << desc->id << ")");
    for (int i = 0; i < desc->properties.size(); ++i) {
        LOG_DBG("\t" << desc->properties[i].name << "(" << desc->properties[i].t.get_name() << ")");
    }
}


}

