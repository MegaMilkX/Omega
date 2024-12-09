#pragma once

#include "gui/elements/viewport/gui_viewport.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_transform.hpp"
#include "csg/csg.hpp"


class GuiViewportToolCsgObjectMode : public GuiViewportToolBase {
    csgScene* csg_scene = 0;
    GuiViewportToolTransform tool_transform;

    void setViewResetPoint() {
        if (!this->viewport) {
            return;
        }
        if (selected_shapes.empty()) {
            return;
        }
        
        gfxm::aabb aabb = selected_shapes[0]->aabb;
        for (int i = 1; i < selected_shapes.size(); ++i) {
            auto shape = selected_shapes[i];
            gfxm::aabb aabb_ = shape->aabb;
            aabb = gfxm::aabb_union(aabb, aabb_);
        }
        gfxm::vec3 shape_mid_point = gfxm::lerp(aabb.from, aabb.to, .5f);
        this->viewport->pivot_reset_point = shape_mid_point;
        float zoom = (aabb.to - shape_mid_point).length() * 1.5f;
        this->viewport->zoom_reset_point = zoom;
    }
public:
    //csgBrushShape* selected_shape = 0;
    std::vector<csgBrushShape*> selected_shapes;

    GuiViewportToolCsgObjectMode(csgScene* csg_scene)
        : GuiViewportToolBase("Object mode"), csg_scene(csg_scene) {
        tool_transform.setOwner(this);
        tool_transform.setParent(this);
    }

    void setViewport(GuiViewport* vp) override { 
        GuiViewportToolBase::setViewport(vp);
        tool_transform.setViewport(vp);
    }
    void selectShape(csgBrushShape* shape, bool add) {
        if (!add) {
            selected_shapes.clear();
        }
        if (shape) {
            selected_shapes.push_back(shape);
            tool_transform.translation = selected_shapes.back()->transform[3];
            tool_transform.rotation = gfxm::to_quat(gfxm::to_mat3(selected_shapes.back()->transform));
        }

        setViewResetPoint();
    }
    void deselectShape(csgBrushShape* shape) {
        auto it = std::find(selected_shapes.begin(), selected_shapes.end(), shape);
        if (it != selected_shapes.end()) {
            selected_shapes.erase(it);

            if (!selected_shapes.empty()) {
                tool_transform.translation = selected_shapes.back()->transform[3];
                tool_transform.rotation = gfxm::to_quat(gfxm::to_mat3(selected_shapes.back()->transform));
            }

            notifyOwner(GUI_NOTIFY::CSG_SHAPE_DESELECTED, shape);
        }
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!selected_shapes.empty()) {
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
            if (!guiIsModifierKeyPressed(GUI_KEY_SHIFT)) {
                selected_shapes.clear();
            }
            gfxm::ray R = viewport->makeRayFromMousePos();
            csgBrushShape* shape = 0;
            csg_scene->pickShape(R.origin, R.origin + R.direction * R.length, &shape);
            
            if (guiIsModifierKeyPressed(GUI_KEY_SHIFT)) {
                auto it = std::find(selected_shapes.begin(), selected_shapes.end(), shape);
                if (it != selected_shapes.end()) {
                    deselectShape(shape);
                } else {
                    notifyOwner(GUI_NOTIFY::CSG_SHAPE_SELECTED, shape);
                    selectShape(shape, guiIsModifierKeyPressed(GUI_KEY_SHIFT));
                }
            } else {
                notifyOwner(GUI_NOTIFY::CSG_SHAPE_SELECTED, shape);
                selectShape(shape, guiIsModifierKeyPressed(GUI_KEY_SHIFT));
            }
            return true;
        }
        case GUI_MSG::RCLICK:
        case GUI_MSG::DBL_RCLICK: {
            selected_shapes.clear();
            setViewResetPoint();
            //notifyOwner(GUI_NOTIFY::CSG_SHAPE_SELECTED, selected_shape);
            return true;
        }
        case GUI_MSG::KEYDOWN: {
            switch (params.getA<uint16_t>()) {
            case 0x51: // Q - flip solidity
                if (!selected_shapes.empty()) {
                    for (int i = 0; i < selected_shapes.size(); ++i) {
                        selected_shapes[i]->volume_type = (selected_shapes[i]->volume_type == CSG_VOLUME_SOLID) ? CSG_VOLUME_EMPTY : CSG_VOLUME_SOLID;
                        csg_scene->invalidateShape(selected_shapes[i]);
                        notifyOwner(GUI_NOTIFY::CSG_SHAPE_CHANGED, selected_shapes[i]);
                    }
                }
                return true;
            case 0x47: // G
                tool_transform.mode_flags = GUI_TRANSFORM_GIZMO_TRANSLATE;
                return true;
            case 0x52: // R
                tool_transform.mode_flags = GUI_TRANSFORM_GIZMO_ROTATE;
                return true;
            case 0x58: // X
                if (!selected_shapes.empty()) {
                    for (int i = 0; i < selected_shapes.size(); ++i) {
                        notifyOwner(GUI_NOTIFY::CSG_SHAPE_DELETE, selected_shapes[i]);
                    }
                    selected_shapes.clear();
                }
                return true;
            case 0x44: // D - copy
                if (guiIsModifierKeyPressed(GUI_KEY_SHIFT)) {
                    if (!selected_shapes.empty()) {
                        std::vector<csgBrushShape*> cloned_shapes;
                        std::vector<csgBrushShape*> sorted_shapes = selected_shapes;
                        std::sort(sorted_shapes.begin(), sorted_shapes.end(), [](const csgBrushShape* a, const csgBrushShape* b)->bool {
                            return a->uid < b->uid;
                        });
                        for (int i = 0; i < sorted_shapes.size(); ++i) {
                            auto shape = new csgBrushShape;
                            shape->clone(sorted_shapes[i]);
                            csg_scene->addShape(shape);
                            cloned_shapes.push_back(shape);
                        }
                        csg_scene->update();
                        notifyOwner(GUI_NOTIFY::CSG_REBUILD, 0);
                        selected_shapes = cloned_shapes;
                    }
                }
                return true;
            }
            break;
        }
        case GUI_MSG::NOTIFY: {
            switch (params.getA<GUI_NOTIFY>()) {   
            case GUI_NOTIFY::TRANSLATION_UPDATE: {
                GuiViewportToolTransform* tool = params.getB<GuiViewportToolTransform*>();
                if (!selected_shapes.empty()) {
                    for (int i = 0; i < selected_shapes.size(); ++i) {
                        selected_shapes[i]->setTransform(
                            gfxm::translate(gfxm::mat4(1.f), tool->delta_translation)
                            * selected_shapes[i]->transform
                        );
                        //notifyOwner(GUI_NOTIFY::CSG_SHAPE_CHANGED, selected_shapes[i]);
                    }
                }
                return true;
            }
            case GUI_NOTIFY::ROTATION_UPDATE: {
                GuiViewportToolTransform* tool = params.getB<GuiViewportToolTransform*>();
                gfxm::mat4 tr = gfxm::translate(gfxm::mat4(1.f), tool->translation);
                gfxm::mat4 invtr = gfxm::inverse(tr);
                if (!selected_shapes.empty()) {
                    for (int i = 0; i < selected_shapes.size(); ++i) {
                        selected_shapes[i]->setTransform(
                            tr * gfxm::to_mat4(tool->delta_rotation) * invtr * selected_shapes[i]->transform
                        );
                    }
                }
                return true;
            }
            case GUI_NOTIFY::TRANSFORM_UPDATE_STOPPED: {
                GuiViewportToolTransform* tool = params.getB<GuiViewportToolTransform*>();
                if (!selected_shapes.empty()) {
                    notifyOwner(GUI_NOTIFY::CSG_SHAPE_CHANGED, selected_shapes.back());
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
        
        if (!selected_shapes.empty()) {
            auto color = gfxm::make_rgba32(1, .5f, 0, 1);
            //guiDrawAABB(selected_shape->aabb, gfxm::mat4(1.f), color);
            
            for (int i = 0; i < selected_shapes.size(); ++i) {
                for (auto& face : selected_shapes[i]->faces) {
                    for (int i = 0; i < face->vertexCount(); ++i) {
                        int j = (i + 1) % face->vertexCount();
                        gfxm::vec3 a = face->getWorldVertexPos(i);
                        gfxm::vec3 b = face->getWorldVertexPos(j);
                        guiDrawLine3(a, b, color);
                    }
                }
                /*
                for (auto& cp : selected_shapes[i]->control_points) {
                    guiDrawPointSquare3d(selected_shapes[i]->world_space_vertices[cp->index], 15.0f, 0xFFFF5555);
                }*/
            }
        }

        guiPopViewTransform();
        guiPopProjection();
        guiPopViewportRect();

        if (!selected_shapes.empty()) {
            tool_transform.onDrawTool(client_area, proj, view);
        }
    }
};
