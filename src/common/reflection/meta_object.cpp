#include "meta_object.hpp"

#include "type_desc.hpp"
#include "type_property_desc.hpp"
#include "reflection.hpp"

#include "log/log.hpp"


namespace rtti {


type MetaObject::get_type() const {
    return type_get<decltype(*this)>();
}

static void makeSnapshotRecur(PropSnapshot& snap, const MetaObject* object, type t) {
    for (int i = 0; i < t.prop_count(); ++i) {
        auto prop_desc = t.get_prop(i);
        rtti::type prop_type = prop_desc->t;
        if (prop_type.is_derived_from(rtti::type_get<rtti::MetaObject>())) {
            // TODO: What if prop is only gettable by value?
            MetaObject* nested_object = prop_desc->fn_as_meta_object(object);
            rtti::PropSnapshot nested_snap;
            nested_object->makeSnapshot(nested_snap);
            snap.add(prop_desc->name, rtti::varying::make(nested_snap), t.get_name());
        } else {
            varying var = t.get_prop_value(object, i);
            snap.add(prop_desc->name, var, t.get_name());
        }
    }

    for (auto& p : t.get_desc()->parent_types) {        
        makeSnapshotRecur(snap, object, p.parent_type);
    }
}

void MetaObject::makeSnapshot(PropSnapshot& snap) const {
    snap.type_ = get_type();
    makeSnapshotRecur(snap, this, snap.type_);
}

static void applySnapshotRecur(const PropSnapshot& snap, MetaObject* object, type t) {
    for (auto& p : t.get_desc()->parent_types) {        
        applySnapshotRecur(snap, object, p.parent_type);
    }

    for (auto& kv : snap.props) {
        const std::string& name = kv.first;
        const varying& var = kv.second;
        rtti::type var_type = var.get_type();

        auto prop_desc = t.get_prop_desc(name);
        if (!prop_desc) {
            LOG_WARN("applySnapshot: Property '" << name << "' does not exist, owner type '" << t.get_name() << "'");
            continue;
        } else {
            LOG_DBG("applySnapshot: Property '" << name << "' exists, owner type '" << t.get_name() << "'");
        }

        if (var_type == rtti::type_get<rtti::PropSnapshot>()) {
            MetaObject* prop_obj = prop_desc->fn_as_meta_object(object);
            if (!prop_obj) {
                LOG_WARN("applySnapshot: Property '" << name << "' is not a MetaObject");
                continue;
            }
            const rtti::PropSnapshot* snap = var.get<rtti::PropSnapshot>();
            if (!snap) {
                LOG_ERR("applySnapshot: Unexpected null snapshot, property: '" << name << "'");
                continue;
            }
            prop_obj->applySnapshot(*snap);
        } else {
            rtti::type prop_type = prop_desc->t;
            if (prop_type != var_type) {
                LOG_WARN("applySnapshot: Property '" << name << "' vs snapshot type mismatch: '" << prop_type.get_name() << "' vs '" << var_type.get_name() << "'");
                continue;
            }
            // TODO:
            t.set_property_unsafe(name.c_str(), object, const_cast<void*>(var.data()));
        }
    }
}
void MetaObject::applySnapshot(const PropSnapshot& snap) {
    type t = get_type();

    applySnapshotRecur(snap, this, t);
    onSnapshot();
}

bool MetaObject::readSnapshot(const nlohmann::json& json, PropSnapshot& out) {
    if (!json.is_object()) {
        LOG_ERR("PropSnapshot json must be an object");
        assert(false);
        return false;
    }

    std::string stype = json.value("@type", "");
    type t = type_get(stype.c_str());
    if (!t.is_valid()) {
        LOG_ERR("Failed to read snapshot, type: '" << t.get_name() << "'");
        assert(false);
        return false;
    }

    if (!t.is_derived_from(rtti::type_get<rtti::MetaObject>())) {
        LOG_ERR("Failed to read snapshot: type '" << t.get_name() << "' is not derived from MetaObject");
        assert(false);
        return false;
    }

    // TODO: Cache schema-generating default object or just it's schema-snapshot
    rtti::MetaObject* default_mo = t.construct_new<rtti::MetaObject>();
            
    rtti::PropSnapshot schema;
    default_mo->makeSnapshot(schema);
    out.clear();
    out.fromJson(schema, json);

    delete default_mo;
    return true;
}

}

