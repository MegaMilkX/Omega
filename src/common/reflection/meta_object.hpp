#pragma once

#include "meta_object.auto.hpp"
#include "type.hpp"
#include "prop_snapshot.hpp"


namespace rtti {


[[cppi_class, no_reflect]];
struct MetaObject {
    virtual ~MetaObject() {}
    virtual type get_type() const;

    virtual void makeSnapshot(PropSnapshot&);
    virtual void applySnapshot(PropSnapshot&);
};


}

