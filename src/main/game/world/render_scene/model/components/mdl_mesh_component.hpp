#pragma once

#include "game/world/render_scene/model/mdl_instance.hpp"

struct mdlMeshComponentMutable : public mdlComponentMutable {
    TYPE_ENABLE(mdlComponentMutable);

    gpuMesh*        mesh = 0;
    gpuMaterial*    material = 0;

    void reflect() override {
        type_register<mdlMeshComponentMutable>("MeshMutable");
    }
};
struct mdlMeshComponentPrototype : public mdlComponentPrototype {
    TYPE_ENABLE(mdlComponentPrototype);

    struct Element {
        int transform_id;
        gpuMesh* mesh;
        gpuMaterial* material;
    };
    std::vector<Element> elements;

    void make(mdlModelPrototype* owner, mdlComponentMutable** m, int count) {
        LOG_WARN("mdlMeshComponentPrototype: " << count);
        elements.resize(count);
        for (int i = 0; i < count; ++i) {
            mdlMeshComponentMutable* mu = (mdlMeshComponentMutable*)m[i];
            elements[i].transform_id = owner->name_to_node[mu->owner->name];
            elements[i].mesh = mu->mesh;
            elements[i].material = mu->material;
        }
    }

    void reflect() override {
        type_register<Element>("MeshPrototypeElement")
            .prop("material", &Element::material)
            .prop("mesh", &Element::mesh)
            .prop("transform_id", &Element::transform_id);
        type_register<mdlMeshComponentPrototype>("MeshPrototype")
            .prop("elements", &mdlMeshComponentPrototype::elements);
    }
};
struct mdlMeshComponentInstance : public mdlComponentInstance {
    TYPE_ENABLE(mdlComponentInstance);
    
    std::vector<std::unique_ptr<scnMeshObject>> objects;

    void make(mdlModelInstance* owner, mdlComponentPrototype* p) {
        LOG_WARN("mdlMeshComponentInstance");
        mdlMeshComponentPrototype* proto = (mdlMeshComponentPrototype*)p;
        objects.resize(proto->elements.size());
        for (int i = 0; i < objects.size(); ++i) {
            objects[i].reset(new scnMeshObject);
            objects[i]->setMeshDesc(proto->elements[i].mesh->getMeshDesc());
            objects[i]->setMaterial(proto->elements[i].material);
            objects[i]->setSkeletonNode(&owner->scn_skeleton, proto->elements[i].transform_id);
        }
    }

    void onSpawn(wWorld* world) {
        for (int i = 0; i < objects.size(); ++i) {
            world->getRenderScene()->addRenderObject(objects[i].get());
        }
    }
    void onDespawn(wWorld* world) {
        for (int i = 0; i < objects.size(); ++i) {
            world->getRenderScene()->removeRenderObject(objects[i].get());
        }
    }

    void reflect() override {
        type_register<mdlMeshComponentInstance>("MeshInstance");
    }
};
typedef ComponentType<mdlMeshComponentMutable, mdlMeshComponentPrototype, mdlMeshComponentInstance> mdlMeshComponent;
