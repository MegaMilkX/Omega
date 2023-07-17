#pragma once

#include "gui/elements/viewport/gui_viewport.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_transform.hpp"
#include "csg/csg.hpp"


class GuiViewportToolCsgObjectMode : public GuiViewportToolBase {
    csgScene* csg_scene = 0;
    GuiViewportToolTransform tool_transform;
public:
    csgBrushShape* selected_shape = 0;

    GuiViewportToolCsgObjectMode(csgScene* csg_scene)
        : GuiViewportToolBase("Object mode"), csg_scene(csg_scene) {
        tool_transform.setOwner(this);
        tool_transform.setParent(this);
    }

    void setViewport(GuiViewport* vp) override { 
        GuiViewportToolBase::setViewport(vp);
        tool_transform.setViewport(vp);
    }
    void selectShape(csgBrushShape* shape) {
        selected_shape = shape;
        if (selected_shape) {
            tool_transform.translation = selected_shape->transform[3];
            tool_transform.rotation = gfxm::to_quat(gfxm::to_mat3(selected_shape->transform));
        }
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (selected_shape) {
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
        case GUI_MSG::LCLICK: {
            selected_shape = 0;
            gfxm::ray R = viewport->makeRayFromMousePos();
            csg_scene->pickShape(R.origin, R.origin + R.direction * R.length, &selected_shape);
            notifyOwner(GUI_NOTIFY::CSG_SHAPE_SELECTED, selected_shape);

            selectShape(selected_shape);
            return true;
        }
        case GUI_MSG::RCLICK: {
            selected_shape = 0;
            notifyOwner(GUI_NOTIFY::CSG_SHAPE_SELECTED, selected_shape);
            return true;
        }
        case GUI_MSG::KEYDOWN: {
            switch (params.getA<uint16_t>()) {
            case 0x47: // G
                tool_transform.mode_flags = GUI_TRANSFORM_GIZMO_TRANSLATE;
                return true;
            case 0x52: // R
                tool_transform.mode_flags = GUI_TRANSFORM_GIZMO_ROTATE;
                return true;
            case 0x58: // X
                if (selected_shape) {
                    notifyOwner(GUI_NOTIFY::CSG_SHAPE_DELETE, selected_shape);
                    selected_shape = 0;
                }
                return true;
            }
            break;
        }
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {   
            case GUI_NOTIFY::TRANSFORM_UPDATED: {
                GuiViewportToolTransform* tool = params.getB<GuiViewportToolTransform*>();
                if (selected_shape) {
                    // TODO: Optimize
                    selected_shape->setTransform(
                        gfxm::translate(gfxm::mat4(1.f), tool->translation)
                        * gfxm::to_mat4(tool->rotation)
                    );
                }
                return true;
            }
            case GUI_NOTIFY::TRANSFORM_UPDATED_STOPPED: {
                GuiViewportToolTransform* tool = params.getB<GuiViewportToolTransform*>();
                if (selected_shape) {
                    selected_shape->setTransform(
                        gfxm::translate(gfxm::mat4(1.f), tool->translation)
                        * gfxm::to_mat4(tool->rotation)
                    );
                    notifyOwner(GUI_NOTIFY::CSG_SHAPE_CHANGED, selected_shape);
                }
                return true;
            }
            }
            break;
        }
        }
        return false;
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
        
        if (selected_shape) {
            auto color = gfxm::make_rgba32(1, .5f, 0, 1);
            guiDrawAABB(selected_shape->aabb, gfxm::mat4(1.f), color);
            /*
            for (auto& face : selected_shape->faces) {
                for (int i = 0; i < face->vertexCount(); ++i) {
                    int j = (i + 1) % face->vertexCount();
                    gfxm::vec3 a = face->getWorldVertexPos(i);
                    gfxm::vec3 b = face->getWorldVertexPos(j);
                    guiDrawLine3(a, b, color);
                }
            }

            for (auto& cp : selected_shape->control_points) {
                guiDrawPointSquare3d(selected_shape->world_space_vertices[cp->index], 15.0f, 0xFFFF5555);
            }*/
        }

        guiPopViewTransform();
        guiPopProjection();
        guiPopViewportRect();

        if (selected_shape) {
            tool_transform.onDrawTool(client_area, proj, view);
        }
    }
};
