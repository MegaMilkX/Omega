#pragma once

#include "gui/elements/viewport/gui_viewport.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_transform.hpp"
#include "csg/csg.hpp"


class GuiViewportToolCsgFaceMode : public GuiViewportToolBase {
    GuiViewportToolTransform tool_transform;
    csgScene* csg_scene = 0;
    csgBrushShape* shape = 0;
    int face_id = -1;

public:
    GuiViewportToolCsgFaceMode()
        : GuiViewportToolBase("Face mode") {
        tool_transform.setOwner(this);
        tool_transform.setParent(this);
    }

    void setViewport(GuiViewport* vp) override {
        GuiViewportToolBase::setViewport(vp);
        tool_transform.setViewport(vp);
    }
    void setShapeData(csgScene* csg_scene, csgBrushShape* shape) {
        this->csg_scene = csg_scene;
        this->shape = shape;
        face_id = -1;
    }
    
    void moveCameraToSelection() {
        assert(viewport);
        if (!viewport) {
            return;
        }
        if (face_id >= 0) {
            assert(face);
            gfxm::aabb box = shape->faces[face_id]->aabb_world;
            gfxm::vec3 new_pivot = gfxm::lerp(box.from, box.to, .5f);
            float new_zoom = gfxm::length(box.to - new_pivot) * 2.f;
            viewport->setCameraPivot(new_pivot, new_zoom);
        } else if(shape) {
            gfxm::aabb box = shape->aabb;
            gfxm::vec3 new_pivot = gfxm::lerp(box.from, box.to, .5f);
            float new_zoom = gfxm::length(box.to - new_pivot) * 2.f;
            viewport->setCameraPivot(new_pivot, new_zoom);
        } else {
            assert(false);
        }
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (shape && face_id >= 0) {
            tool_transform.onHitTest(hit, x, y);
            if (hit.hasHit()) {
                return;
            }
        }
        hit.add(GUI_HIT::CLIENT, this);
        return;
    }
    bool onMessage(GUI_MSG msg, GUI_MSG_PARAMS params) override {
        switch (msg) {
        case GUI_MSG::FOCUS:
            return true;
        case GUI_MSG::UNFOCUS:
            return true;
        case GUI_MSG::LCLICK:
        case GUI_MSG::DBL_LCLICK: {
            gfxm::ray R = viewport->makeRayFromMousePos();
            face_id = csg_scene->pickShapeFace(R.origin, R.origin + R.direction * R.length, shape);
            if (face_id >= 0) {
                gfxm::vec3 face_mid_point = shape->faces[face_id]->mid_point;
                tool_transform.translation = face_mid_point;
                tool_transform.rotation = gfxm::quat_identity;
            }
            return true;
        }
        case GUI_MSG::RCLICK:
        case GUI_MSG::DBL_RCLICK: {
            face_id = -1;
            gfxm::vec3 shape_mid_point = gfxm::lerp(shape->aabb.from, shape->aabb.to, .5f);
            return true;
        }
        case GUI_MSG::KEYDOWN: {
            switch (params.getA<uint16_t>()) {
            case 90: // Z - move camera to selected
                moveCameraToSelection();
                return true;
            case 0x47: // G
                tool_transform.mode_flags = GUI_TRANSFORM_GIZMO_TRANSLATE;
                return true;
            case 0x52: // R
                tool_transform.mode_flags = GUI_TRANSFORM_GIZMO_ROTATE;
                return true;
            }
            break;
        }
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {   
            case GUI_NOTIFY::TRANSFORM_UPDATE: {
                if (shape && face_id >= 0) {
                    if (!shape->transformFace(
                        face_id, gfxm::translate(gfxm::mat4(1.f), tool_transform.translation)
                    )) {
                        tool_transform.translation = shape->faces[face_id]->mid_point;
                    }
                }
                return true;
            }
            case GUI_NOTIFY::TRANSFORM_UPDATE_STOPPED: {
                if (shape && face_id >= 0) {
                    notifyOwner(GUI_NOTIFY::CSG_SHAPE_CHANGED, shape);
                }
                return true;
            }
            }
            break;
        }
        }
        return GuiViewportToolBase::onMessage(msg, params);
    }
    void onLayout(const gfxm::rect& rc, uint64_t flags) override {
        tool_transform.projection = projection;
        tool_transform.view = view;
        tool_transform.layout(rc, flags);
    }

    void onDrawTool(const gfxm::rect& client_area, const gfxm::mat4& proj, const gfxm::mat4& view) override {
        guiPushViewportRect(client_area); // TODO: Do this automatically
        guiPushProjection(proj);
        guiPushViewTransform(view);

        if (shape) {
            auto color = gfxm::make_rgba32(1, .5f, 0, 1);
            auto poly_color = gfxm::make_rgba32(1, .5f, 0, .5f);
            for (auto& face : shape->faces) {
                for (int i = 0; i < face->vertexCount(); ++i) {
                    int j = (i + 1) % face->vertexCount();
                    gfxm::vec3 a = face->getWorldVertexPos(i);
                    gfxm::vec3 b = face->getWorldVertexPos(j);
                    guiDrawLine3(a, b, color);
                }
                guiDrawPointSquare3d(face->mid_point, 15.0f, color);
            }
            if (face_id >= 0) {
                std::vector<gfxm::vec3> vertices;
                auto& face = shape->faces[face_id];
                for (int i = 0; i < face->vertexCount(); ++i) {
                    auto& v = face->getWorldVertexPos(i);
                    vertices.push_back(v);
                }
                guiDrawPolyConvex3d(vertices.data(), vertices.size(), poly_color);
            }
        }

        guiPopViewTransform();
        guiPopProjection();
        guiPopViewportRect();

        if (shape && face_id >= 0) {
            tool_transform.onDrawTool(client_area, proj, view);
        }
    }
};