#pragma once

#include "gui/elements/viewport/tools/gui_viewport_tool_base.hpp"
#include "csg/csg.hpp"


typedef uint32_t GUI_TRANSFORM_GIZMO_MODE_FLAGS;
constexpr GUI_TRANSFORM_GIZMO_MODE_FLAGS GUI_TRANSFORM_GIZMO_TRANSLATE = 0x1;
constexpr GUI_TRANSFORM_GIZMO_MODE_FLAGS GUI_TRANSFORM_GIZMO_ROTATE = 0x2;

class GuiViewportToolTransform : public GuiViewportToolBase {
    enum INTERACTION_MODE {
        INTERACTION_NONE,
        INTERACTION_TRANSLATE_AXIS,
        INTERACTION_TRANSLATE_PLANE,
        INTERACTION_ROTATE_AXIS
    } interaction_mode;

    GUI_TRANSFORM_GIZMO_MODE_FLAGS last_used_mode_flags = 0;

    const float snap_step = .125f;
    const float rotation_snap_step = gfxm::radian(11.25f);

    GIZMO_TRANSFORM_STATE gizmo_state;

    float gizmo_scale = 1.f;
    int plane_id_hovered = 0;
    int spin_axis_id_hovered = 0;
    float dxz = .0f;
    float dyz = .0f;
    float dzz = .0f;
    bool is_dragging = false;
    gfxm::vec2 last_mouse_pos;
    gfxm::vec3 translation_axis = gfxm::vec3(1,0,0);
    gfxm::vec3 translation_origin;
    gfxm::vec3 translation_plane_normal;
    gfxm::vec3 translation_axis_offs;
    gfxm::vec3 rotation_axis_ref;
    gfxm::quat rotation_ref;
    float angle_offs = .0f;
    float angle_accum = .0f;

    gfxm::ray getMouseRay(gfxm::vec2 mouse_pos) {
        gfxm::vec2 vpsz(client_area.max.x - client_area.min.x, client_area.max.y - client_area.min.y);
        return gfxm::ray_viewport_to_world(
            vpsz, gfxm::vec2(mouse_pos.x, vpsz.y - mouse_pos.y),
            projection, view
        );
    }
    bool isAnyControlHovered() const {
        return gizmo_state.hovered_axis != 0x0;
    }
    gfxm::mat3 getOrientation() {
        return gfxm::to_mat3(rotation);
    }
    gfxm::mat4 getTransform() {
        return gfxm::translate(gfxm::mat4(1.f), translation)
            * gfxm::to_mat4(rotation);
    }

    float snapRotation(float f) {
        const float inv_snap_step = 1.f / rotation_snap_step;
        return roundf(f * inv_snap_step) * rotation_snap_step;
    }
    gfxm::vec3 snapVec3(const gfxm::vec3& v3) {
        gfxm::vec3 out = v3;
        const float inv_snap_step = 1.f / snap_step;
        out.x = roundf(v3.x * inv_snap_step) * snap_step;
        out.y = roundf(v3.y * inv_snap_step) * snap_step;
        out.z = roundf(v3.z * inv_snap_step) * snap_step;
        return out;
    }

public:
    GUI_TRANSFORM_GIZMO_MODE_FLAGS mode_flags = GUI_TRANSFORM_GIZMO_TRANSLATE;
    gfxm::vec3 base_translation;
    float base_angle = .0f;
    gfxm::vec3 translation;
    gfxm::quat rotation;
    gfxm::vec3 delta_translation;
    gfxm::quat delta_rotation;

    GuiViewportToolTransform()
        : GuiViewportToolBase("Transform") {}

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        const gfxm::mat4 model = getTransform();
        if (last_used_mode_flags != mode_flags) {
            gizmo_state.hovered_axis = 0x0;
            plane_id_hovered = 0;
            spin_axis_id_hovered = 0;
            last_used_mode_flags = mode_flags;
        }

        if (!is_dragging) {
            /*
            if (mode_flags & GUI_TRANSFORM_GIZMO_TRANSLATE) {
                {
                    plane_id_hovered = 0;

                    gfxm::vec3 pt;
                    gfxm::ray R = getMouseRay(gfxm::vec2(x, y) - client_area.min);
                    if (gfxm::intersect_line_plane_point(R.origin, R.direction, model[0], gfxm::dot(gfxm::vec3(model[3]), gfxm::vec3(model[0])), pt)) {
                        gfxm::vec2 pt2d = gfxm::project_point_yz(gfxm::to_mat3(model), model[3], pt);
                        if (pt2d.x > .0f && pt2d.x < .25f * gizmo_scale && pt2d.y > .0f && pt2d.y < .25f * gizmo_scale) {
                            plane_id_hovered = 1;
                        }
                    }
                    if (gfxm::intersect_line_plane_point(R.origin, R.direction, model[1], gfxm::dot(gfxm::vec3(model[3]), gfxm::vec3(model[1])), pt)) {
                        gfxm::vec2 pt2d = gfxm::project_point_xz(gfxm::to_mat3(model), model[3], pt);
                        if (pt2d.x > .0f && pt2d.x < .25f * gizmo_scale && pt2d.y > .0f && pt2d.y < .25f * gizmo_scale) {
                            plane_id_hovered = 2;
                        }
                    }
                    if (gfxm::intersect_line_plane_point(R.origin, R.direction, model[2], gfxm::dot(gfxm::vec3(model[3]), gfxm::vec3(model[2])), pt)) {
                        gfxm::vec2 pt2d = gfxm::project_point_xy(gfxm::to_mat3(model), model[3], pt);
                        if (pt2d.x > .0f && pt2d.x < .25f * gizmo_scale && pt2d.y > .0f && pt2d.y < .25f * gizmo_scale) {
                            plane_id_hovered = 3;
                        }
                    }

                    if (plane_id_hovered != 0) {
                        axis_id_hovered = 0;
                        spin_axis_id_hovered = 0;
                    }
                }
            }*/

            if (mode_flags & GUI_TRANSFORM_GIZMO_ROTATE)
            {
                spin_axis_id_hovered = 0;
                gfxm::mat4 inv_view = gfxm::inverse(view);

                gfxm::vec3 pt;
                float distance_to_cam = INFINITY;
                gfxm::ray R = getMouseRay(gfxm::vec2(x, y) - client_area.min);
                if (gfxm::intersect_line_plane_point(R.origin, R.direction, model[0], gfxm::dot(gfxm::vec3(model[3]), gfxm::vec3(model[0])), pt)) {
                    gfxm::vec2 pt2d = gfxm::project_point_yz(gfxm::to_mat3(model), model[3], pt);
                    float len = pt2d.length();
                    float dist = (gfxm::vec3(inv_view[3]) - pt).length2();
                    if (len < 1.1f * gizmo_scale && len > .9f * gizmo_scale) {
                        spin_axis_id_hovered = 1;
                        distance_to_cam = dist;
                    }
                }
                if (gfxm::intersect_line_plane_point(R.origin, R.direction, model[1], gfxm::dot(gfxm::vec3(model[3]), gfxm::vec3(model[1])), pt)) {
                    gfxm::vec2 pt2d = gfxm::project_point_xz(gfxm::to_mat3(model), model[3], pt);
                    float len = pt2d.length();
                    float dist = (gfxm::vec3(inv_view[3]) - pt).length2();
                    if (len < 1.1f * gizmo_scale && len > .9f * gizmo_scale && dist < distance_to_cam) {
                        spin_axis_id_hovered = 2;
                        distance_to_cam = dist;
                    }
                }
                if (gfxm::intersect_line_plane_point(R.origin, R.direction, model[2], gfxm::dot(gfxm::vec3(model[3]), gfxm::vec3(model[2])), pt)) {
                    gfxm::vec2 pt2d = gfxm::project_point_xy(gfxm::to_mat3(model), model[3], pt);
                    float len = pt2d.length();
                    float dist = (gfxm::vec3(inv_view[3]) - pt).length2();
                    if (len < 1.1f * gizmo_scale && len > .9f * gizmo_scale && dist < distance_to_cam) {
                        spin_axis_id_hovered = 3;
                        distance_to_cam = dist;
                    }
                }/*
                gfxm::vec3 planeN = gfxm::normalize(gfxm::vec3(gfxm::inverse(view)[2]) - gfxm::vec3(model[3]));
                if (gfxm::intersect_line_plane_point(R.origin, R.direction, planeN, gfxm::dot(gfxm::vec3(model[3]), planeN), pt)) {
                    gfxm::vec2 pt2d = gfxm::project_point_xy(gfxm::to_mat3(model), model[3], pt);
                    float len = pt2d.length();
                    if (len < 1.25f && len > 1.05f) {
                        spin_axis_id_hovered = 4;
                    }
                }*/
            }

            if (mode_flags & GUI_TRANSFORM_GIZMO_TRANSLATE) {
                gizmoHitTranslate(
                    gizmo_state,
                    client_area.max.x - client_area.min.x,
                    client_area.max.y - client_area.min.y,
                    x, y
                );
            }

            if (!isAnyControlHovered()) {
                return;
            }
        } else {
            return;
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    float display_angle = .0f;
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::MOUSE_MOVE: {
            const gfxm::mat4 model = getTransform();
            gfxm::vec2 mouse_pos = guiGetMousePosLocal(getGlobalClientArea().min);
            if (is_dragging) {
                gfxm::ray R = getMouseRay(mouse_pos);

                if ((mode_flags & GUI_TRANSFORM_GIZMO_TRANSLATE) && gizmo_state.hovered_axis) {
                    gfxm::vec3 ptA, ptB;
                    gfxm::closest_point_line_line(
                        translation_origin, translation_origin + translation_axis,
                        R.origin, R.origin + R.direction * 1000.f, 
                        ptA, ptB
                    );

                    // Figure out scale modifier necessary to keep gizmo the same size on screen at any distance
                    const float target_size = .2f; // screen ratio
                    float scale = 1.f;
                    {
                        gfxm::vec4 ref4 = model[3];
                        ref4 = projection * view * gfxm::vec4(ref4, 1.f);
                        scale = target_size * ref4.w;
                    }

                    gfxm::vec3 new_pos = ptA - translation_axis_offs;
                    delta_translation = new_pos - base_translation;
                    delta_translation = gfxm::inverse(gfxm::to_mat4(rotation)) * gfxm::vec4(delta_translation, .0f);
                    delta_translation = snapVec3(delta_translation);
                    delta_translation = gfxm::to_mat4(rotation) * gfxm::vec4(delta_translation, .0f);
                    translation = base_translation + delta_translation;// gfxm::vec4(ptA - translation_axis_offs, 1.f);
                    base_translation = translation;
                    notifyOwner(GUI_NOTIFY::TRANSLATION_UPDATE, this);
                    notifyOwner(GUI_NOTIFY::TRANSFORM_UPDATE, this);
                } else if(plane_id_hovered) {
                    gfxm::vec3 pt;
                    gfxm::intersect_line_plane_point(R.origin, R.direction, translation_plane_normal, gfxm::dot(translation_plane_normal, translation_origin), pt);
                    gfxm::vec3 new_pos = pt - translation_axis_offs;
                    delta_translation = new_pos - base_translation;
                    delta_translation = gfxm::inverse(gfxm::to_mat4(rotation)) * gfxm::vec4(delta_translation, .0f);
                    delta_translation = snapVec3(delta_translation);
                    delta_translation = gfxm::to_mat4(rotation) * gfxm::vec4(delta_translation, .0f);
                    translation = base_translation + delta_translation;
                    base_translation = translation;
                    notifyOwner(GUI_NOTIFY::TRANSLATION_UPDATE, this);
                    notifyOwner(GUI_NOTIFY::TRANSFORM_UPDATE, this);
                } else if(spin_axis_id_hovered) {
                    if (spin_axis_id_hovered == 1) {
                        gfxm::vec3 pt;
                        gfxm::intersect_line_plane_point(R.origin, R.direction, model[0], gfxm::dot(gfxm::vec3(model[0]), gfxm::vec3(model[3])), pt);
                        gfxm::vec3 a = rotation_axis_ref;
                        gfxm::vec3 b = gfxm::normalize(pt - gfxm::vec3(model[3]));
                        float angle = acosf(gfxm::dot(a, b) / gfxm::sqrt(a.length() * b.length()));
                        float side = gfxm::dot(gfxm::vec3(model[0]), gfxm::cross(b, a)) > .0f ? -1.f : 1.f;
                        angle *= side;
                        //float tmp = angle;
                        angle -= angle_offs;
                        //angle_offs = tmp;
                        angle = snapRotation(angle);
                        angle_accum += angle;
                        delta_rotation = gfxm::angle_axis(angle - base_angle, gfxm::vec3(model[0]));
                        rotation = delta_rotation * rotation;
                        base_angle = angle;
                        display_angle = angle;
                    } else if (spin_axis_id_hovered == 2) {
                        gfxm::vec3 pt;
                        gfxm::intersect_line_plane_point(R.origin, R.direction, model[1], gfxm::dot(gfxm::vec3(model[1]), gfxm::vec3(model[3])), pt);
                        gfxm::vec3 a = rotation_axis_ref;
                        gfxm::vec3 b = gfxm::normalize(pt - gfxm::vec3(model[3]));
                        float angle = acosf(gfxm::dot(a, b) / gfxm::sqrt(a.length() * b.length()));
                        float side = gfxm::dot(gfxm::vec3(model[1]), gfxm::cross(b, a)) > .0f ? -1.f : 1.f;
                        angle *= side;
                        //float tmp = angle;
                        angle -= angle_offs;
                        //angle_offs = tmp;
                        angle = snapRotation(angle);
                        angle_accum += angle;
                        delta_rotation = gfxm::angle_axis(angle - base_angle, gfxm::vec3(model[1]));
                        rotation = delta_rotation * rotation;
                        base_angle = angle;
                        display_angle = angle;
                    } else if (spin_axis_id_hovered == 3) {
                        gfxm::vec3 pt;
                        gfxm::intersect_line_plane_point(R.origin, R.direction, model[2], gfxm::dot(gfxm::vec3(model[2]), gfxm::vec3(model[3])), pt);
                        gfxm::vec3 a = rotation_axis_ref;
                        gfxm::vec3 b = gfxm::normalize(pt - gfxm::vec3(model[3]));
                        float angle = acosf(gfxm::dot(a, b) / gfxm::sqrt(a.length() * b.length()));
                        float side = gfxm::dot(gfxm::vec3(model[2]), gfxm::cross(b, a)) > .0f ? -1.f : 1.f;
                        angle *= side;
                        //float tmp = angle;
                        angle -= angle_offs;
                        //angle_offs = tmp;
                        angle = snapRotation(angle);
                        angle_accum += angle;
                        delta_rotation = gfxm::angle_axis(angle - base_angle, gfxm::vec3(model[2]));
                        rotation = delta_rotation * rotation;
                        base_angle = angle;
                        display_angle = angle;
                    }
                    notifyOwner(GUI_NOTIFY::ROTATION_UPDATE, this);
                    notifyOwner(GUI_NOTIFY::TRANSFORM_UPDATE, this);
                }
            }
            last_mouse_pos = mouse_pos;
            viewport->sendMessage(msg, params);
            } return true;
        case GUI_MSG::LBUTTON_DOWN:
            is_dragging = true;
            guiCaptureMouse(this);
            if ((mode_flags & GUI_TRANSFORM_GIZMO_TRANSLATE) && gizmo_state.hovered_axis) {
                const gfxm::mat4 model = getTransform();
                interaction_mode = INTERACTION_TRANSLATE_AXIS;
                ///assert(axis_id_hovered && axis_id_hovered <= 3);
                translation_axis = gfxm::normalize(model[gizmo_state.hovered_axis - 1]);
                translation_origin = model[3];
                gfxm::ray R = getMouseRay(last_mouse_pos);
                gfxm::vec3 ptA, ptB;
                gfxm::closest_point_line_line(
                    translation_origin, translation_origin + translation_axis,
                    R.origin, R.origin + R.direction * 1000.f,
                    ptA, ptB
                );
                translation_axis_offs = ptA - translation_origin;

                base_translation = translation;

                gizmo_state.is_active = true;
            } else if(plane_id_hovered) {
                const gfxm::mat4 model = getTransform();
                translation_plane_normal = model[plane_id_hovered - 1];
                translation_origin = model[3];
                gfxm::ray R = getMouseRay(last_mouse_pos);
                gfxm::intersect_line_plane_point(R.origin, R.direction, translation_plane_normal, gfxm::dot(translation_plane_normal, translation_origin), translation_axis_offs);
                translation_axis_offs = translation_axis_offs - translation_origin;

                base_translation = translation;
            } else if(spin_axis_id_hovered) {
                const gfxm::mat4 model = getTransform();
                gfxm::vec3 pt;
                gfxm::ray R = getMouseRay(last_mouse_pos);
                rotation_ref = rotation;
                angle_accum = .0f;
                if (spin_axis_id_hovered == 1) {
                    rotation_axis_ref = gfxm::normalize(model[1]);
                    gfxm::intersect_line_plane_point(R.origin, R.direction, model[0], gfxm::dot(gfxm::vec3(model[0]), gfxm::vec3(model[3])), pt);
                    gfxm::vec3 a = rotation_axis_ref;
                    gfxm::vec3 b = gfxm::normalize(pt - gfxm::vec3(model[3]));
                    angle_offs = acosf(gfxm::dot(a, b) / gfxm::sqrt(a.length() * b.length()));
                    float side = gfxm::dot(gfxm::vec3(model[0]), gfxm::cross(b, a)) > .0f ? -1.f : 1.f;
                    angle_offs *= side;
                } else if (spin_axis_id_hovered == 2) {
                    rotation_axis_ref = gfxm::normalize(model[0]);
                    gfxm::intersect_line_plane_point(R.origin, R.direction, model[1], gfxm::dot(gfxm::vec3(model[1]), gfxm::vec3(model[3])), pt);
                    gfxm::vec3 a = rotation_axis_ref;
                    gfxm::vec3 b = gfxm::normalize(pt - gfxm::vec3(model[3]));
                    angle_offs = acosf(gfxm::dot(a, b) / gfxm::sqrt(a.length() * b.length()));
                    float side = gfxm::dot(gfxm::vec3(model[1]), gfxm::cross(b, a)) > .0f ? -1.f : 1.f;
                    angle_offs *= side;
                } else if (spin_axis_id_hovered == 3) {
                    rotation_axis_ref = gfxm::normalize(model[1]);
                    gfxm::intersect_line_plane_point(R.origin, R.direction, model[2], gfxm::dot(gfxm::vec3(model[2]), gfxm::vec3(model[3])), pt);
                    gfxm::vec3 a = rotation_axis_ref;
                    gfxm::vec3 b = gfxm::normalize(pt - gfxm::vec3(model[3]));
                    angle_offs = acosf(gfxm::dot(a, b) / gfxm::sqrt(a.length() * b.length()));
                    float side = gfxm::dot(gfxm::vec3(model[2]), gfxm::cross(b, a)) > .0f ? -1.f : 1.f;
                    angle_offs *= side;
                }

                base_angle = .0f;
            }
            return true;
        case GUI_MSG::LBUTTON_UP:
            if (is_dragging) {
                notifyOwner(GUI_NOTIFY::TRANSFORM_UPDATE_STOPPED, this);
            }
            is_dragging = false;
            guiCaptureMouse(0);
            interaction_mode = INTERACTION_NONE;
            gizmo_state.is_active = false;
            return true;
        }
        return GuiViewportToolBase::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        rc_bounds = gfxm::rect(gfxm::vec2(0, 0), extents);
        client_area = rc_bounds;
    }
    
    void onDrawTool(const gfxm::rect& client_area, const gfxm::mat4& proj, const gfxm::mat4& view) override {
        assert(viewport);

        auto gizmo_ctx = viewport->render_instance->gizmo_ctx.get();
        assert(gizmo_ctx);

        const gfxm::mat4 model = getTransform();
        gizmo_state.transform = model;
        gizmo_state.projection = proj;
        gizmo_state.view = view;

        // Translator
        if (mode_flags & GUI_TRANSFORM_GIZMO_TRANSLATE) {
            gizmoTranslate(gizmo_ctx, gizmo_state);
        }

        // Rotator
        if (mode_flags & GUI_TRANSFORM_GIZMO_ROTATE) {
            gizmoRotate(gizmo_ctx, gizmo_state);
        }

        guiDrawText(client_area.min + gfxm::vec2(10, 30), MKSTR("angle: " << display_angle).c_str(), getFont(), 0, 0xFFFFFFFF);
    }
};
