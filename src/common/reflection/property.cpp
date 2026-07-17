#include "property.hpp"
#include "type.hpp"
#include "type_property_desc.hpp"
#include "varying.hpp"
#include "log/log.hpp"


const std::string& property::get_name() const {
    type object_type = type(object_type_uid);
    auto prop_desc = object_type.get_prop(prop_idx);
    return prop_desc->name;
}
type property::get_type() const {
    type object_type = type(object_type_uid);
    auto prop_desc = object_type.get_prop(prop_idx);
    return prop_desc->t;
}
void property::set(MetaObject* object, const varying& var) {
    type object_type = type(object_type_uid);
    auto prop_desc = object_type.get_prop(prop_idx);
    if (prop_desc->t != var.get_type()) {
        LOG_ERR("TYPE: property::set: property and varying must have the same type, conversion not yet supported");
        assert(false);
        return;
    }
    if (!prop_desc->fn_set) {
        LOG_ERR("TYPE: property::set: not assignable, missing fn_set()");
        assert(false);
        return;
    }
    prop_desc->fn_set(object, var.data());
}
varying property::get(MetaObject* object) {
    type object_type = type(object_type_uid);
    auto prop_desc = object_type.get_prop(prop_idx);
    return prop_desc->get_value(object);
}

