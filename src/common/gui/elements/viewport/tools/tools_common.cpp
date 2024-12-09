#include "tools_common.hpp"



void guiViewportToolCsgPickSurface(const CSG_PICK_PARAMS& params) {
    auto viewport = params.viewport;
    auto csg_scene = params.csg_scene;
    float snap_step = params.snap_step;
    gfxm::vec3 cursor3d_pos = gfxm::vec3(0, 0, 0);
    gfxm::vec3 cursor3d_normal = gfxm::vec3(0, 1, 0);
    gfxm::mat3 cursor3d_orient = gfxm::mat3(1.f);
    csgBrushShape* picked_shape = 0;
    csgFace* picked_face = 0;
    int picked_face_idx = -1;

    
    gfxm::vec3 origin = gfxm::vec3(0, 0, 0);
    gfxm::ray R = viewport->makeRayFromMousePos();
    gfxm::vec3 pos;
    gfxm::vec3 N;
    gfxm::vec3 plane_origin;
    gfxm::mat3 orient = gfxm::mat3(1);
        
    picked_face_idx = csg_scene->pickFace(R.origin, R.origin + R.direction * R.length, &picked_shape, pos, N, plane_origin, orient);
    if (picked_face_idx >= 0) {
        cursor3d_normal = N;
        origin = plane_origin;
    } else if(gfxm::intersect_line_plane_point(R.origin, R.direction, gfxm::vec3(0, 1, 0), .0f, pos)) {
        N = gfxm::vec3(0, 1, 0);
        cursor3d_normal = N;
    }
    // Calc orientation matrix
    {
        cursor3d_orient = orient;
            
        const gfxm::vec3 right = gfxm::vec3(1, 0, 0);
        const gfxm::vec3 up = gfxm::vec3(0, 1, 0);
        const gfxm::vec3 back = gfxm::vec3(0, 0, 1);

        if (fabsf(gfxm::dot(N, up)) < 1.f - FLT_EPSILON) {
            gfxm::vec3 new_right = gfxm::normalize(gfxm::cross(up, N));
            gfxm::vec3 new_up = gfxm::normalize(gfxm::cross(-new_right, N));
            cursor3d_orient[0] = new_right;
            cursor3d_orient[1] = new_up;
            cursor3d_orient[2] = N;
        } else {
            gfxm::vec3 new_right = gfxm::normalize(gfxm::cross(-back, N));
            gfxm::vec3 new_up = gfxm::normalize(gfxm::cross(-new_right, N));
            cursor3d_orient[0] = new_right;
            cursor3d_orient[1] = new_up;
            cursor3d_orient[2] = N;
        }

        gfxm::vec2 pos2d = gfxm::project_point_xy(cursor3d_orient, origin, pos);
        const float inv_snap_step = 1.f / snap_step;
        pos2d.x = roundf(pos2d.x * inv_snap_step) * snap_step;
        pos2d.y = roundf(pos2d.y * inv_snap_step) * snap_step;
        pos = gfxm::unproject_point_xy(pos2d, origin, cursor3d_orient[0], cursor3d_orient[1]);
        cursor3d_pos = pos;
    }

    if (params.out_normal) {
        *params.out_normal = cursor3d_normal;
    }
    if (params.out_position) {
        *params.out_position = cursor3d_pos;
    }
    if (params.out_orient) {
        *params.out_orient = cursor3d_orient;
    }
    if (params.out_shape) {
        *params.out_shape = picked_shape;
    }
    if (params.out_face) {
        *params.out_face = picked_face;
    }
    if (params.out_face_idx) {
        *params.out_face_idx = picked_face_idx;
    }
}

void guiViewportToolCsgDrawCursor3d(const gfxm::vec3& pos, const gfxm::mat3& orient) {
    guiDrawLine3(pos - orient[0] * .5f, pos + orient[0] * .5f, 0xFFFFFFFF);
    guiDrawLine3(pos - orient[1] * .5f, pos + orient[1] * .5f, 0xFFFFFFFF);
}