#include "meta_object.hpp"

#include "type_desc.hpp"
#include "type_property_desc.hpp"


namespace rtti {


static void makeSnapshotRecur(PropSnapshot& snap, MetaObject* object, type t) {
    for (int i = 0; i < t.prop_count(); ++i) {
        auto prop_desc = t.get_prop(i);
        varying var = t.get_prop_value(object, i);
        snap.add(prop_desc->name, var, t.get_name());
    }

    for (auto& p : t.get_desc()->parent_types) {        
        makeSnapshotRecur(snap, object, p.parent_type);
    }
}

void MetaObject::makeSnapshot(PropSnapshot& snap) {
    type t = get_type();

    makeSnapshotRecur(snap, this, t);
}

static void applySnapshotRecur(PropSnapshot& snap, MetaObject* object, type t) {
    for (auto& p : t.get_desc()->parent_types) {        
        applySnapshotRecur(snap, object, p.parent_type);
    }

    for (auto& kv : snap.props) {
        const std::string& name = kv.first;
        const varying& var = kv.second;

        // TODO:
        t.set_property_unsafe(name.c_str(), object, const_cast<void*>(var.data()));
    }
}
void MetaObject::applySnapshot(PropSnapshot& snap) {
    type t = get_type();

    applySnapshotRecur(snap, this, t);
    onSnapshot();
}


}

