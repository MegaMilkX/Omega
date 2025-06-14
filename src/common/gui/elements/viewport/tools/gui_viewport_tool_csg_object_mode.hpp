#pragma once

#include "gui/elements/viewport/gui_viewport.hpp"
#include "gui/elements/viewport/tools/gui_viewport_tool_transform.hpp"
#include "csg/csg.hpp"


class GuiViewportToolCsgObjectMode : public GuiViewportToolBase {
    csgScene* csg_scene = 0;
    GuiViewportToolTransform tool_transform;
public:
    std::vector<csgObject*> selected_objects;

    GuiViewportToolCsgObjectMode(csgScene* csg_scene)
        : GuiViewportToolBase("Object mode"), csg_scene(csg_scene) {
        tool_transform.setOwner(this);
        tool_transform.setParent(this);
    }

    void setViewport(GuiViewport* vp) override { 
        GuiViewportToolBase::setViewport(vp);
        tool_transform.setViewport(vp);
    }
    void selectObject(csgObject* shape, bool add) {
        if (!add) {
            selected_objects.clear();
        }
        if (shape) {
            selected_objects.push_back(shape);
            tool_transform.translation = selected_objects.back()->transform[3];
            tool_transform.rotation = gfxm::to_quat(gfxm::to_mat3(selected_objects.back()->transform));
        }
    }
    void deselectObject(csgObject* shape) {
        auto it = std::find(selected_objects.begin(), selected_objects.end(), shape);
        if (it != selected_objects.end()) {
            selected_objects.erase(it);

            if (!selected_objects.empty()) {
                tool_transform.translation = selected_objects.back()->transform[3];
                tool_transform.rotation = gfxm::to_quat(gfxm::to_mat3(selected_objects.back()->transform));
            }

            notifyOwner(GUI_NOTIFY::CSG_SHAPE_DESELECTED, shape);
        }
    }

    void moveCameraToSelection() {
        assert(viewport);
        if (!viewport) {
            return;
        }
        if (selected_objects.empty()) {
            return;
        }
        gfxm::aabb box = selected_objects[0]->aabb;
        for (int i = 1; i < selected_objects.size(); ++i) {
            gfxm::expand_aabb(box, selected_objects[i]->aabb.from);
            gfxm::expand_aabb(box, selected_objects[i]->aabb.to);
        }
        gfxm::vec3 new_pivot = gfxm::lerp(box.from, box.to, .5f);
        float new_zoom = gfxm::length(box.to - new_pivot) * 2.f;
        viewport->setCameraPivot(new_pivot, new_zoom);
    }

    void groupSelection() {
        if (selected_objects.size() < 2) {
            return;
        }
        
        auto group = new csgGroupObject(selected_objects.data(), (int)selected_objects.size());
        csg_scene->addObject(group);

        notifyOwner(GUI_NOTIFY::CSG_SHAPE_SELECTED, group);
        selectObject(group, false);
    }
    void ungroup(csgGroupObject* group) {
        selected_objects.clear();
        for (int i = 0; i < group->objectCount(); ++i) {
            selected_objects.push_back(group->getObject(i));
        }
        group->dissolve();
        csg_scene->removeObject(group);
    }
    void toggleGroupSelection() {
        if (selected_objects.size() == 1) {
            csgGroupObject* group = dynamic_cast<csgGroupObject*>(selected_objects[0]);
            if (group) {
                ungroup(group);
                return;
            }
            return;
        }
        groupSelection();
    }

    void onHitTest(GuiHitResult& hit, int x, int y) override {
        if (!selected_objects.empty()) {
            tool_transform.hitTest(hit, x, y);
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
                selected_objects.clear();
            }
            gfxm::ray R = viewport->makeRayFromMousePos();
            csgBrushShape* shape = 0;
            csg_scene->pickShape(R.origin, R.origin + R.direction * R.length, &shape);
            csgObject* object = shape;
            while (object && object->owner) {
                object = object->owner;
            }
            
            if (guiIsModifierKeyPressed(GUI_KEY_SHIFT)) {
                auto it = std::find(selected_objects.begin(), selected_objects.end(), object);
                if (it != selected_objects.end()) {
                    deselectObject(object);
                } else {
                    notifyOwner(GUI_NOTIFY::CSG_SHAPE_SELECTED, object);
                    selectObject(object, guiIsModifierKeyPressed(GUI_KEY_SHIFT));
                }
            } else {
                notifyOwner(GUI_NOTIFY::CSG_SHAPE_SELECTED, object);
                selectObject(object, guiIsModifierKeyPressed(GUI_KEY_SHIFT));
            }
            return true;
        }
        case GUI_MSG::RCLICK:
        case GUI_MSG::DBL_RCLICK: {
            selected_objects.clear();
            //notifyOwner(GUI_NOTIFY::CSG_SHAPE_SELECTED, selected_shape);
            return true;
        }
        case GUI_MSG::KEYDOWN: {
            switch (params.getA<uint16_t>()) {
            case 90: // Z - move camera to selected
                moveCameraToSelection();
                return true;
            case 0x51: // Q - flip solidity
                if (!selected_objects.empty()) {
                    for (int i = 0; i < selected_objects.size(); ++i) {
                        csgBrushShape* shape = dynamic_cast<csgBrushShape*>(selected_objects[i]);
                        if (shape) {
                            shape->volume_type = (shape->volume_type == CSG_VOLUME_SOLID) ? CSG_VOLUME_EMPTY : CSG_VOLUME_SOLID;
                            csg_scene->invalidateShape(shape);
                            notifyOwner(GUI_NOTIFY::CSG_SHAPE_CHANGED, shape);
                        }
                    }
                }
                return true;
            case 0x47: // G
                if (guiIsModifierKeyPressed(GUI_KEY_SHIFT)) {
                    toggleGroupSelection();
                } else {
                    tool_transform.mode_flags = GUI_TRANSFORM_GIZMO_TRANSLATE;
                }
                return true;
            case 0x52: // R
                tool_transform.mode_flags = GUI_TRANSFORM_GIZMO_ROTATE;
                return true;
            case 0x58: // X
                if (!selected_objects.empty()) {
                    for (int i = 0; i < selected_objects.size(); ++i) {
                        notifyOwner(GUI_NOTIFY::CSG_SHAPE_DELETE, selected_objects[i]);
                    }
                    selected_objects.clear();
                }
                return true;
            case 0x44: // D - copy
                if (guiIsModifierKeyPressed(GUI_KEY_SHIFT)) {
                    if (!selected_objects.empty()) {
                        std::vector<csgObject*> cloned_objects;
                        std::vector<csgObject*> sorted_objects = selected_objects;
                        std::sort(sorted_objects.begin(), sorted_objects.end(), [](const csgObject* a, const csgObject* b)->bool {
                            return a->uid < b->uid;
                        });
                        for (int i = 0; i < sorted_objects.size(); ++i) {
                            auto obj = sorted_objects[i]->makeCopy();
                            if (obj) {
                                csg_scene->addObject(obj);
                                cloned_objects.push_back(obj);
                            } else {
                                LOG_WARN("csgObject::makeCopy() returned null");
                            }
                        }
                        csg_scene->update();
                        notifyOwner(GUI_NOTIFY::CSG_REBUILD, 0);
                        selected_objects = cloned_objects;
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
                if (!selected_objects.empty()) {
                    for (int i = 0; i < selected_objects.size(); ++i) {
                        selected_objects[i]->translateRelative(tool->delta_translation, tool->rotation);
                        /*
                        selected_objects[i]->setTransform(
                            gfxm::translate(gfxm::mat4(1.f), tool->delta_translation)
                            * selected_objects[i]->transform
                        );*/
                        //notifyOwner(GUI_NOTIFY::CSG_SHAPE_CHANGED, selected_shapes[i]);
                    }
                }
                return true;
            }
            case GUI_NOTIFY::ROTATION_UPDATE: {
                GuiViewportToolTransform* tool = params.getB<GuiViewportToolTransform*>();
                gfxm::mat4 tr = gfxm::translate(gfxm::mat4(1.f), tool->translation);
                gfxm::mat4 invtr = gfxm::inverse(tr);
                if (!selected_objects.empty()) {
                    for (int i = 0; i < selected_objects.size(); ++i) {
                        selected_objects[i]->rotateRelative(tool->delta_rotation, tool->translation);
                        /*
                        selected_objects[i]->setTransform(
                            tr * gfxm::to_mat4(tool->delta_rotation) * invtr * selected_objects[i]->transform
                        );*/
                    }
                }
                return true;
            }
            case GUI_NOTIFY::TRANSFORM_UPDATE_STOPPED: {
                GuiViewportToolTransform* tool = params.getB<GuiViewportToolTransform*>();
                if (!selected_objects.empty()) {
                    notifyOwner(GUI_NOTIFY::CSG_SHAPE_CHANGED, selected_objects.back());
                }
                return true;
            }
            }
            break;
        }
        }
        return GuiViewportToolBase::onMessage(msg, params);
    }
    void onLayout(const gfxm::vec2& extents, uint64_t flags) override {
        tool_transform.projection = projection;
        tool_transform.view = view;
        tool_transform.layout_position = gfxm::vec2(0, 0);
        tool_transform.layout(extents, flags);
    }

    void drawShape(csgBrushShape* shape, uint32_t color) {
        if (!shape) {
            return;
        }
        
        auto gizmo_ctx = viewport->render_instance->gizmo_ctx.get();
        assert(gizmo_ctx);
        
        for (auto& face : shape->faces) {
            for (int i = 0; i < face->vertexCount(); ++i) {
                int j = (i + 1) % face->vertexCount();
                gfxm::vec3 a = face->getWorldVertexPos(i);
                gfxm::vec3 b = face->getWorldVertexPos(j);
                // TODO: LINE THICKNESS
                gizmoLine(gizmo_ctx, a, b, .020f, color);
            }
        }
    }
    void drawGroup(csgGroupObject* group, uint32_t color) {
        for (int j = 0; j < group->objectCount(); ++j) {
            auto obj = group->getObject(j);
            csgBrushShape* shape = dynamic_cast<csgBrushShape*>(obj);
            csgGroupObject* group = dynamic_cast<csgGroupObject*>(obj);
            if (shape) {
                drawShape(shape, color);
            } else if(group) {
                drawGroup(group, color);
            }
        }
    }
    void onDrawTool(const gfxm::rect& client_area, const gfxm::mat4& proj, const gfxm::mat4& view) override {       
        if (!selected_objects.empty()) {
            auto color = gfxm::make_rgba32(1, .5f, 0, 1);
            auto color_group = gfxm::make_rgba32(.3f, .6f, 1.f, 1.f);
            //guiDrawAABB(selected_shape->aabb, gfxm::mat4(1.f), color);
            
            for (int i = 0; i < selected_objects.size(); ++i) {
                csgBrushShape* shape = dynamic_cast<csgBrushShape*>(selected_objects[i]);
                csgGroupObject* group = dynamic_cast<csgGroupObject*>(selected_objects[i]);
                if (shape) {
                    drawShape(shape, color);
                } else if(group) {
                    drawGroup(group, color_group);
                }
                /*
                for (auto& cp : selected_shapes[i]->control_points) {
                    guiDrawPointSquare3d(selected_shapes[i]->world_space_vertices[cp->index], 15.0f, 0xFFFF5555);
                }*/
            }
        }

        if (!selected_objects.empty()) {
            tool_transform.onDrawTool(client_area, proj, view);
        }
    }
};
