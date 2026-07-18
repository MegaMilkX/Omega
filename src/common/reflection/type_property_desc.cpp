#include "type_property_desc.hpp"
#include "varying.hpp"


namespace rtti {


varying type_property_desc::get_value(const MetaObject* object) const {
    if (!fn_get_varying) {
        return varying();
    }
    return fn_get_varying(object);
}


}

