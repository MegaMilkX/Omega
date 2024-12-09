#pragma once

#include "gui/elements/viewport/gui_viewport.hpp"
#include "csg/csg.hpp"



class GuiViewportToolCsgCreateBox : public GuiViewportToolBase {
    csgScene* csg_scene = 0;

    gfxm::vec3 cursor3d_pos = gfxm::vec3(0, 0, 0);
    gfxm::vec3 cursor3d_normal = gfxm::vec3(0, 1, 0);
    gfxm::mat3 cursor3d_orient = gfxm::mat3(1.f);

    enum SHAPE_TYPE {
        SHAPE_TYPE_BOX,
        SHAPE_TYPE_CYLINDER
    } shape_type = SHAPE_TYPE_BOX;

    enum BOX_CREATE_STATE {
        BOX_CREATE_NONE,
        BOX_CREATE_XY,
        BOX_CREATE_Z
    } box_create_state = BOX_CREATE_NONE;
    gfxm::vec3 box_ref_point;
    gfxm::vec3 box_corner_a;
    gfxm::vec3 box_corner_b;
    float box_height;
    gfxm::mat3 box_orient = gfxm::mat3(1.f);
    gfxm::mat3 cyl_orient = gfxm::mat3(1.f);
    CSG_VOLUME_TYPE box_volume = CSG_VOLUME_SOLID;
    gfxm::vec3 box_size;
    const float snap_step = .125f;
public:
    GuiViewportToolCsgCreateBox(csgScene* csg_scene)
        : GuiViewportToolBase("Create box"), csg_scene(csg_scene) {}
    void onHitTest(GuiHitResult& hit, int x, int y) override {
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::FOCUS:
            return true;
        case GUI_MSG::UNFOCUS:
            return true;
        case GUI_MSG::KEYDOWN: {
            switch (params.getA<uint16_t>()) {
            case 0x31: // 1 key
                if (box_create_state != BOX_CREATE_NONE) {
                    return false;
                }
                shape_type = SHAPE_TYPE_BOX;
                return true;
            case 0x32: // 2 key
                if (box_create_state != BOX_CREATE_NONE) {
                    return false;
                }
                shape_type = SHAPE_TYPE_CYLINDER;
                return true;
            }
            return false;
        }
        case GUI_MSG::LCLICK: {
            switch (box_create_state) {
            case BOX_CREATE_NONE:
                //selected_shape = 0;
                box_create_state = BOX_CREATE_XY;
                box_ref_point = cursor3d_pos;
                box_corner_a = cursor3d_pos;
                box_corner_b = box_corner_a;
                box_height = 0;
                box_orient = cursor3d_orient;
                cyl_orient[0] = cursor3d_orient[1];
                cyl_orient[1] = cursor3d_orient[2];
                cyl_orient[2] = cursor3d_orient[0];
                box_size = gfxm::vec3(0, 0, 0);
                break;
            case BOX_CREATE_XY: {
                box_create_state = BOX_CREATE_Z;
                gfxm::ray R = viewport->makeRayFromMousePos();
                gfxm::vec3 pos;
                float D = gfxm::dot(box_corner_a, box_orient[2]);
                if (!gfxm::intersect_line_plane_point(R.origin, R.direction, box_orient[2], D, pos)) {
                    // TODO: Technically possible, need to handle
                    box_corner_b = box_corner_a + gfxm::vec3(1, 1, 1);
                } else {
                    box_corner_b = pos;
                }
                break;
            }
            case BOX_CREATE_Z: {
                box_create_state = BOX_CREATE_NONE;
                box_height = 1.f;// viewport.cursor3d_pos.y - box_corner_b.y;
                gfxm::vec2 box_xy = gfxm::project_point_xy(box_orient, box_corner_a, box_corner_b);
                //box_size.z = box_height;

                auto ptr = new csgBrushShape;
                switch (shape_type) {
                case SHAPE_TYPE_BOX:
                    csgMakeBox(ptr, box_size.x, box_size.y, box_size.z,
                        gfxm::translate(gfxm::mat4(1.f), box_corner_a/*(box_corner_a + box_corner_b) * .5f*/)
                        * gfxm::to_mat4(box_orient)
                    );
                    break;
                case SHAPE_TYPE_CYLINDER:
                    csgMakeCylinder(ptr, box_size.z, gfxm::length(gfxm::vec2(box_size.x, box_size.y)), 16,
                        gfxm::translate(gfxm::mat4(1.f), box_corner_a) * gfxm::to_mat4(cyl_orient)
                    );
                    break;
                default:
                    assert(false);
                }

                if (guiIsModifierKeyPressed(GUI_KEY_CONTROL)) {
                    box_volume = CSG_VOLUME_EMPTY;
                } else {
                    box_volume = CSG_VOLUME_SOLID;
                }
                ptr->volume_type = box_volume;
                ptr->rgba = 0xFFFFFFFF;// gfxm::make_rgba32((rand() % 100) * .01f, (rand() % 100) * .01f, (rand() % 100) * .01f, 1.f);
                owner->sendMessage(GUI_MSG::NOTIFY, GUI_NOTIFY::CSG_SHAPE_CREATED, ptr);
                owner->sendMessage(GUI_MSG::NOTIFY, GUI_NOTIFY::VIEWPORT_TOOL_DONE, (GuiViewportToolBase*)this);
                break;
            }
            }
            return true;
        }
        case GUI_MSG::RCLICK: {
            switch (box_create_state) {
            case BOX_CREATE_NONE: {
                owner->sendMessage(GUI_MSG::NOTIFY, GUI_NOTIFY::VIEWPORT_TOOL_DONE, (GuiViewportToolBase*)this);
                break;
            }
            case BOX_CREATE_XY: {
                box_create_state = BOX_CREATE_NONE;
                break;
            }
            case BOX_CREATE_Z: {
                box_create_state = BOX_CREATE_XY;
                box_height = .0f;
                box_size.z = .0f;
                break;
            }
            }
            return true;
        }
        case GUI_MSG::MOUSE_MOVE: {
            switch (box_create_state) {
            case BOX_CREATE_XY: {
                gfxm::ray R = viewport->makeRayFromMousePos();
                gfxm::vec3 pos;
                float D = gfxm::dot(box_corner_a, box_orient[2]);
                if (!gfxm::intersect_line_plane_point(R.origin, R.direction, box_orient[2], D, pos)) {
                    // TODO: Technically possible, need to handle
                    box_corner_b = box_corner_a + gfxm::vec3(1, 1, 1);
                } else {
                    box_corner_b = pos;
                    if (guiIsModifierKeyPressed(GUI_KEY_ALT)) {
                        box_corner_a = box_ref_point + box_ref_point - pos;
                    } else {
                        box_corner_a = box_ref_point;
                    }
                }
                gfxm::vec2 box_xy = gfxm::project_point_xy(box_orient, box_corner_a, box_corner_b);
                const float inv_snap_step = 1.f / snap_step;
                box_xy.x = roundf(box_xy.x * inv_snap_step) * snap_step;
                box_xy.y = roundf(box_xy.y * inv_snap_step) * snap_step;
                box_size.x = box_xy.x;
                box_size.y = box_xy.y;
                break;
            }
            case BOX_CREATE_Z: {
                gfxm::ray R = viewport->makeRayFromMousePos();
                gfxm::vec3 box_mid = box_corner_a + (box_corner_b - box_corner_a) * .5f;
                gfxm::vec3 boxN = box_orient[2];
                gfxm::vec3 A;
                gfxm::vec3 B;
                gfxm::closest_point_line_line(
                    R.origin, R.origin + R.direction,
                    box_corner_b, box_corner_b + boxN,
                    A, B
                );
                float t = gfxm::dot(B - box_mid, boxN);
                const float inv_snap_step = 1.f / snap_step;
                t = roundf(t * inv_snap_step) * snap_step;
                box_height = t;
                box_size.z = t;
                break;
            }
            }
            viewport->sendMessage(msg, params);
            return true;
        }
        }
        return GuiViewportToolBase::onMessage(msg, params);
    }
    void onDrawTool(const gfxm::rect& client_area, const gfxm::mat4& proj, const gfxm::mat4& view) override {
        gfxm::vec3 origin = gfxm::vec3(0, 0, 0);
        gfxm::ray R = viewport->makeRayFromMousePos();
        gfxm::vec3 pos;
        gfxm::vec3 N;
        gfxm::vec3 plane_origin;
        gfxm::mat3 orient = gfxm::mat3(1);
        if (csg_scene->castRay(R.origin, R.origin + R.direction * R.length, pos, N, plane_origin, orient)) {
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

        guiPushViewportRect(client_area); // TODO: Do this automatically
        guiPushProjection(proj);
        guiPushViewTransform(view);

        if (box_create_state == BOX_CREATE_NONE) {
            guiDrawLine3(cursor3d_pos - cursor3d_orient[0] * .5f, cursor3d_pos + cursor3d_orient[0] * .5f, 0xFFFFFFFF);
            guiDrawLine3(cursor3d_pos - cursor3d_orient[1] * .5f, cursor3d_pos + cursor3d_orient[1] * .5f, 0xFFFFFFFF);
        } else if (box_create_state != BOX_CREATE_NONE) {
            if (shape_type == SHAPE_TYPE_BOX) {
                auto tr = gfxm::translate(gfxm::mat4(1.f), box_corner_a)
                    * gfxm::to_mat4(box_orient);
                guiDrawAABB(gfxm::aabb(gfxm::vec3(0, 0, 0), box_size), tr, 0xFFFFFFFF);
            } else if(shape_type == SHAPE_TYPE_CYLINDER) {
                guiDrawCylinder3(gfxm::length(gfxm::vec2(box_size.x, box_size.y)), box_size.z, gfxm::translate(gfxm::mat4(1.f), box_corner_a) * gfxm::to_mat4(cyl_orient), 0xFFFFFFFF);
            }
        }

        guiPopViewTransform();
        guiPopProjection();
        guiPopViewportRect();
        /*
        guiDrawText(
            client_area.min + gfxm::vec2(GUI_MARGIN, GUI_MARGIN),
            MKSTR("[" << cursor3d_pos.x << ", " << cursor3d_pos.y << ", " << cursor3d_pos.z << "]").c_str(),
            guiGetCurrentFont(), 0, 0xFFFFFFFF
        );*/
    }
};
