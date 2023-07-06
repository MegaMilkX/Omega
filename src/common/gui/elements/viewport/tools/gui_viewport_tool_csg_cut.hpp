#pragma once

#include "gui/elements/viewport/gui_viewport.hpp"
#include "csg/csg.hpp"



class GuiViewportToolCsgCut : public GuiViewportToolBase {
    csgScene* csg_scene = 0;
    csgBrushShape* shape = 0;

    enum CUT_STATE {
        CUT_STATE_NONE,
        CUT_STATE_PREVIEW
    } state = CUT_STATE_NONE;
    int face_id = -1;
    gfxm::vec3 ref_point;
    gfxm::vec3 ref_normal;
    gfxm::vec3 cut_plane_N;
    float cut_plane_D;
    csgShapeCutData cut_data;
public:
    GuiViewportToolCsgCut(csgScene* csg_scene)
        : GuiViewportToolBase("Cut shape"), csg_scene(csg_scene) {}

    void setData(csgBrushShape* shape) {
        this->shape = shape;


        gfxm::ray R = viewport->makeRayFromMousePos();
        gfxm::vec3 hit;
        face_id = csg_scene->pickShapeFace(R.origin, R.origin + R.direction * R.length, shape, &hit);
        if (face_id == -1) {
            return;
        }
        ref_point = hit;
        ref_normal = shape->faces[face_id]->N;
        cut_data.clear();
        state = CUT_STATE_PREVIEW;
    }

    GuiHitResult onHitTest(int x, int y) override {
        return GuiHitResult{ GUI_HIT::CLIENT, this };
    }

    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::LCLICK: {
            switch (state) {
            case CUT_STATE_NONE: {
                if (face_id == -1) {
                    break;
                }
                cut_data.clear();
                state = CUT_STATE_PREVIEW;
                break;
            }
            case CUT_STATE_PREVIEW: {
                csgPerformCut(cut_data);
                notifyOwner(GUI_NOTIFY::CSG_SHAPE_CHANGED, (csgBrushShape*)shape);
                notifyOwner(GUI_NOTIFY::VIEWPORT_TOOL_DONE, (GuiViewportToolBase*)this);
                break;
            }
            }
            return true;
        }
        case GUI_MSG::RCLICK: {
            switch (state) {
            case CUT_STATE_NONE: {
                notifyOwner(GUI_NOTIFY::VIEWPORT_TOOL_DONE, (GuiViewportToolBase*)this);
                break;
            }
            case CUT_STATE_PREVIEW: {
                state = CUT_STATE_NONE;
                notifyOwner(GUI_NOTIFY::VIEWPORT_TOOL_DONE, (GuiViewportToolBase*)this);
                break;
            }
            }
            return true;
        }
        case GUI_MSG::MOUSE_MOVE: {
            switch (state) {
            case CUT_STATE_NONE: {
                gfxm::ray R = viewport->makeRayFromMousePos();
                gfxm::vec3 hit;
                face_id = csg_scene->pickShapeFace(R.origin, R.origin + R.direction * R.length, shape, &hit);
                if (face_id == -1) {
                    break;
                }
                ref_point = hit;
                ref_normal = shape->faces[face_id]->N;
                break;
            }
            case CUT_STATE_PREVIEW: {
                gfxm::ray R = viewport->makeRayFromMousePos();
                gfxm::vec3 hit;
                if (!gfxm::intersect_line_plane_point(R.origin, R.direction, ref_normal, gfxm::dot(ref_normal, ref_point), hit)) {
                    break;
                }
                cut_plane_N = gfxm::normalize(gfxm::cross(ref_normal, hit - ref_point));
                cut_plane_D = gfxm::dot(cut_plane_N, ref_point);
                csgPrepareCut(shape, cut_plane_N, cut_plane_D, cut_data);
                break;
            }
            }
        }
        }
        return false;
    }
    void onDrawTool(const gfxm::rect& client_area, const gfxm::mat4& proj, const gfxm::mat4& view) override {
        guiPushViewportRect(client_area); // TODO: Do this automatically
        guiPushProjection(proj);
        guiPushViewTransform(view);

        if (state == CUT_STATE_PREVIEW) {
            if (!cut_data.preview_lines.empty()) {
                for (int i = 0; i < cut_data.preview_lines.size() - 1; i += 2) {
                    int j = i + 1;
                    auto& p0 = cut_data.preview_lines[i];
                    auto& p1 = cut_data.preview_lines[j];
                    guiDrawLine3(p0, p1, 0xFFFFFFFF);
                }

                for (int i = 0; i < cut_data.front_control_points.size(); ++i) {
                    auto cp = cut_data.front_control_points[i];
                    guiDrawPointSquare3d(shape->world_space_vertices[cp->index], 15.0f, 0xFF0000FF);
                }
                for (int i = 0; i < cut_data.back_control_points.size(); ++i) {
                    auto cp = cut_data.back_control_points[i];
                    guiDrawPointSquare3d(shape->world_space_vertices[cp->index], 15.0f, 0xFF00FF00);
                }
                for (int i = 0; i < cut_data.aligned_control_points.size(); ++i) {
                    auto cp = cut_data.aligned_control_points[i];
                    guiDrawPointSquare3d(shape->world_space_vertices[cp->index], 15.0f, 0xFFFF0000);
                }
            }
        }

        guiPopViewTransform();
        guiPopProjection();
        guiPopViewportRect();
    }
};