#include "csg_object.hpp"

#include "../csg_brush_shape.hpp"
#include "csg_group_object.hpp"
#include "csg_custom_shape_object.hpp"


csgObject* csgCreateObjectFromTypeName(const std::string& type) {
    if (type == "csgBrushShape") {
        return new csgBrushShape;
    } else if(type == "csgGroupObject") {
        return new csgGroupObject;
    } else if(type == "csgCustomShapeObject") {
        return new csgCustomShapeObject;
    } else {
        LOG_ERR("Unknown CSG object type: " << type);
        assert(false);
        return 0;
    }
}