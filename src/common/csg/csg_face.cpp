#include "csg_face.hpp"

#include "csg_brush_shape.hpp"

const gfxm::vec3& csgFace::getLocalVertexPos(int i) const { 
    return control_points[i]->position; 
}
const gfxm::vec3& csgFace::getWorldVertexPos(int i) const { 
    return shape->world_space_vertices[control_points[i]->index]; 
}