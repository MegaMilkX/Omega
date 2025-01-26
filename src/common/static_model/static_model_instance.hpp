#pragma once

#include "render_scene/render_object/scn_mesh_object.hpp"
#include "render_scene/render_scene.hpp"


class StaticModel;
class StaticModelInstance {
    StaticModel* static_model = 0;

    Handle<TransformNode> transform;
    std::vector<std::unique_ptr<scnMeshObject>> mesh_objects;
public:
    StaticModelInstance() {}
    StaticModelInstance(StaticModel* static_model);
    StaticModelInstance(StaticModelInstance&& other);
    ~StaticModelInstance();

    StaticModelInstance& operator=(StaticModelInstance&& other) {
        static_model = other.static_model;
        transform = other.transform;
        mesh_objects = std::move(other.mesh_objects);
        return *this;
    }

    void setTransformNode(Handle<TransformNode> transform) {
        this->transform = transform;
        for (int i = 0; i < mesh_objects.size(); ++i) {
            mesh_objects[i]->setTransformNode(transform);
        }
    }

    void spawn(scnRenderScene* scn);
    void despawn(scnRenderScene* scn);
};