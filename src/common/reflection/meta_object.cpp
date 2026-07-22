#include "meta_object.hpp"

#include "type_desc.hpp"
#include "type_property_desc.hpp"


namespace rtti {


void MetaObject::makeSnapshot(PropSnapshot& snap) {
    type t = get_type();
    snap.type_ = t;
    snap.props.clear();

    for (int i = 0; i < t.prop_count(); ++i) {
        auto prop_desc = t.get_prop(i);
        varying var = t.get_prop_value(this, i);
        snap.add(prop_desc->name, var, "");
    }
}

void MetaObject::applySnapshot(PropSnapshot& snap) {
    type t = get_type();

    for (auto& kv : snap.props) {
        const std::string& name = kv.first;
        const varying& var = kv.second;

        // TODO:
        t.set_property_unsafe(name.c_str(), this, const_cast<void*>(var.data()));
    }
}


}

