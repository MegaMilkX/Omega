#pragma once

#include "collision_contact_point.hpp"
#include "collision_triangle_mesh.hpp"


void fixEdgeCollisionNormal(ContactPoint& cp, int tri, const gfxm::mat4& mesh_transform, const CollisionTriangleMesh* mesh);

