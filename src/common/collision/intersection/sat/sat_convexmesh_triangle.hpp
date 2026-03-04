#pragma once

#include "../../shape/convex_mesh.hpp"


bool SAT_ConvexMeshTriangle(const gfxm::mat4& mesh_transform, const phyConvexMesh* mesh, const gfxm::vec3* tri, phyContactPoint& out_contact);

