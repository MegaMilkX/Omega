#pragma once

#include "gui/elements/viewport/gui_viewport.hpp"
#include "csg/csg.hpp"
#include "tools_common.hpp"


class GuiViewportToolCsgUVEdit : public GuiViewportToolBase {
    csgScene* csg_scene = 0;

    enum STATE {
        STATE_BEGIN,
        STATE_STRETCH
    };
    STATE state = STATE_BEGIN;

    bool has_mouse_moved = false;

    struct Point {
        gfxm::mat3 orient;
        gfxm::vec3 position;
        gfxm::vec3 normal;
    };
    Point point_a;
    gfxm::vec3 point_b;
    csgBrushShape* picked_shape = 0;
    int picked_face_idx = -1;

    const float snap_step = .125f;

    gfxm::vec3 cursor3d_pos = gfxm::vec3(0, 0, 0);
    gfxm::vec3 cursor3d_normal = gfxm::vec3(0, 1, 0);
    gfxm::mat3 cursor3d_orient = gfxm::mat3(1.f);

    void adjustUV() {
        gfxm::vec2 projected_b = gfxm::project_point_xy(point_a.orient, point_a.position, point_b);
        auto face = picked_shape->faces[picked_face_idx].get();
        for (int i = 0; i < face->control_points.size(); ++i) {
            gfxm::vec3 p3d = face->getWorldVertexPos(i);// control_points[i]->position;
            gfxm::vec2 p2d = gfxm::project_point_xy(point_a.orient, point_a.position, p3d);
            p2d.x /= projected_b.x;
            p2d.y /= -projected_b.y;
            //face->control_points[i]->uv = p2d; // TODO: Does nothing. Is it unused?
            face->uvs[i].x = p2d.x;
            face->uvs[i].y = p2d.y;
        }
        face->uv_offset = gfxm::vec2(0, 0);
        face->uv_scale = gfxm::vec2(1, 1);
        picked_shape->automatic_uv = false;
        picked_shape->markForRebuild();
        csg_scene->update();
        owner->sendMessage(GUI_MSG::NOTIFY, GUI_NOTIFY::CSG_SHAPE_CHANGED, picked_shape);
    }
public:
    GuiViewportToolCsgUVEdit(csgScene* scene)
    : GuiViewportToolBase("UV edit"),
    csg_scene(scene) {}

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LCLICK: {
            if (state == STATE_BEGIN) {
                state = STATE_STRETCH;
                point_a.normal = cursor3d_normal;
                point_a.orient = cursor3d_orient;
                point_a.position = cursor3d_pos;

                // Align orientation to view
                gfxm::mat3 view_orient = gfxm::to_mat3(gfxm::inverse(viewport->getViewTransform()));
                float dx = gfxm::dot(view_orient[0], point_a.orient[0]);
                float dy = gfxm::dot(view_orient[1], point_a.orient[1]);
                float dz = gfxm::dot(view_orient[2], point_a.orient[2]);
                if (dx < gfxm::radian(45.f) && dx > .0f) {
                    std::swap(point_a.orient[0], point_a.orient[1]);
                } else if(dx < .0f) {
                    point_a.orient[0] *= -1.f;
                    point_a.orient[1] *= -1.f;
                }

            } else if(state == STATE_STRETCH) {
                state = STATE_BEGIN;
                //adjustUV();
            }
            return true;
        }
        case GUI_MSG::RCLICK: {
            if (state == STATE_BEGIN) {
                owner->sendMessage(GUI_MSG::NOTIFY, GUI_NOTIFY::VIEWPORT_TOOL_DONE, (GuiViewportToolBase*)this);
            } else if(state == STATE_STRETCH) {
                state = STATE_BEGIN;
            }
            return true;
        }
        case GUI_MSG::MOUSE_MOVE: {
            if (state == STATE_STRETCH) {
                gfxm::ray R = viewport->makeRayFromMousePos();
                gfxm::vec3 pos;
                float D = gfxm::dot(point_a.position, point_a.orient[2]);
                if (!gfxm::intersect_line_plane_point(R.origin, R.direction, point_a.orient[2], D, pos)) {
                    // TODO: Technically possible, need to handle
                    //box_corner_b = box_corner_a + gfxm::vec3(1, 1, 1);
                } else {
                    point_b = pos;
                }
                gfxm::vec2 pos2d = gfxm::project_point_xy(point_a.orient, point_a.position, pos);
                const float inv_snap_step = 1.f / snap_step;
                pos2d.x = roundf(pos2d.x * inv_snap_step) * snap_step;
                pos2d.y = roundf(pos2d.y * inv_snap_step) * snap_step;
                pos = gfxm::unproject_point_xy(pos2d, point_a.position, point_a.orient[0], point_a.orient[1]);
                point_b = pos;
                has_mouse_moved = true;
            }
            viewport->sendMessage(msg, params);
            return true;
        }
        }
        return GuiViewportToolBase::onMessage(msg, params);
    };
    void onDrawTool(const gfxm::rect& client_area, const gfxm::mat4& proj, const gfxm::mat4& view) override {        
        if(state == STATE_BEGIN) {
            CSG_PICK_PARAMS params = { 0 };
            params.csg_scene = csg_scene;
            params.snap_step = snap_step;
            params.viewport = viewport;
            params.out_position = &cursor3d_pos;
            params.out_normal = &cursor3d_normal;
            params.out_orient = &cursor3d_orient;
            params.out_shape = &picked_shape;
            params.out_face_idx = &picked_face_idx;
            guiViewportToolCsgPickSurface(params);
        }

        guiPushViewportRect(client_area); // TODO: Do this automatically
        guiPushProjection(proj);
        guiPushViewTransform(view);

        if (state == STATE_BEGIN) {
            guiViewportToolCsgDrawCursor3d(cursor3d_pos, cursor3d_orient);
        } else if (state == STATE_STRETCH) {
            guiDrawLine3(point_a.position - point_a.orient[0] * .5f, point_a.position + point_a.orient[0] * .5f, 0xFF0000FF);
            guiDrawLine3(point_a.position - point_a.orient[1] * .5f, point_a.position + point_a.orient[1] * .5f, 0xFF00FF00);

            guiDrawLine3(point_b - point_a.orient[0] * .5f, point_b + point_a.orient[0] * .5f, 0xFFFFFFFF);
            guiDrawLine3(point_b - point_a.orient[1] * .5f, point_b + point_a.orient[1] * .5f, 0xFFFFFFFF);
        }

        guiPopViewTransform();
        guiPopProjection();
        guiPopViewportRect();

        if (has_mouse_moved) {
            adjustUV();
            has_mouse_moved = false;
        }

        // TODO: Draw face
        /*
        for (int i = 0; i < face->control_points.size(); ++i) {
            gfxm::vec3 a = selected_shape->world_space_vertices[face->control_points[i]->index];
            gfxm::vec3 b = a + face->normals[i] * .2f;
            guiDrawLine3(a, b, 0xFFFF0000);
        }*/
    }
};
