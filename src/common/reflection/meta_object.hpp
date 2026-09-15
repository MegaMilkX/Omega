#pragma once

#include "meta_object.auto.hpp"
#include "nlohmann/json.hpp"
#include "type.hpp"
#include "prop_snapshot.hpp"


namespace rtti {


[[cppi_class, no_reflect]];
struct MetaObject {
    virtual ~MetaObject() {}
    virtual type get_type() const;

    virtual void makeSnapshot(PropSnapshot&) const;
    virtual void applySnapshot(const PropSnapshot&);
    virtual void onSnapshot() {}

    static bool readSnapshot(const nlohmann::json& json, PropSnapshot& out);
};


}

