#pragma once

#include <set>
#include <unordered_set>
#include <unordered_map>
#include "gpu/gpu_texture_2d.hpp"
#include "gpu/gpu_material.hpp"
#include "math/gfxm.hpp"

#include "csg_vertex.hpp"
#include "csg_plane.hpp"
#include "csg_fragment.hpp"
#include "csg_face.hpp"
#include "csg_brush_shape.hpp"
#include "csg_scene.hpp"
#include "csg_triangulation.hpp"


void csgMakeCube(csgBrushShape* shape, float width, float height, float depth, const gfxm::mat4& transform);
void csgMakeBox(csgBrushShape* shape, float width, float height, float depth, const gfxm::mat4& transform);
void csgMakeCylinder(csgBrushShape* shape, float height, float radius, int segments, const gfxm::mat4& transform);
void csgMakeSphere(csgBrushShape* shape, int segments, float radius, const gfxm::mat4& tranform);


