#pragma once

#include "meta_object.auto.hpp"
#include "type.hpp"


namespace rtti {


[[cppi_class, no_reflect]];
struct MetaObject {
    virtual ~MetaObject() {}
    virtual type get_type() const;
};


}

