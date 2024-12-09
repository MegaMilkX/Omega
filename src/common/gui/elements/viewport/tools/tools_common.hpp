#pragma once

#include "math/gfxm.hpp"
#include "gui/elements/viewport/gui_viewport.hpp"
#include "csg/csg_scene.hpp"


struct CSG_PICK_PARAMS {
    GuiViewport* viewport;
    csgScene* csg_scene;
    float snap_step;
    gfxm::mat3* out_orient;
    gfxm::vec3* out_position;
    gfxm::vec3* out_normal;
    csgBrushShape** out_shape;
    csgFace** out_face;
    int* out_face_idx;
};
void guiViewportToolCsgPickSurface(const CSG_PICK_PARAMS& params);

void guiViewportToolCsgDrawCursor3d(const gfxm::vec3& pos, const gfxm::mat3& orient);