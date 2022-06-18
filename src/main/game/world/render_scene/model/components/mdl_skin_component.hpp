#pragma once

#include "game/world/render_scene/model/mdl_instance.hpp"

struct mdlSkinComponentMutable : public mdlComponentMutable {
    TYPE_ENABLE(mdlComponentMutable);

    std::vector<mdlNode*> bones;
    std::vector<gfxm::mat4> inverse_bind_transforms;
    HSHARED<gpuMesh>        mesh;
    HSHARED<gpuMaterial>    material;

    void reflect() override {
        type_register<mdlSkinComponentMutable>("SkinMutable")
            .parent<mdlComponentMutable>();
    }
};
struct mdlSkinComponentPrototype : public mdlComponentPrototype {
    TYPE_ENABLE(mdlComponentPrototype);
    
    struct Element {
        std::vector<int> bone_indices;
        std::vector<gfxm::mat4> inverse_bind_transforms;
        HSHARED<gpuMesh>        mesh;
        HSHARED<gpuMaterial>    material;
    };
    std::vector<Element> elements;

    void make(mdlModelPrototype* owner, mdlComponentMutable** m, int count) {
        LOG_WARN("mdlSkinComponentPrototype: " << count);
        elements.resize(count);
        for (int i = 0; i < count; ++i) {
            mdlSkinComponentMutable* mu = (mdlSkinComponentMutable*)m[i];
            elements[i].bone_indices.resize(mu->bones.size());
            for (int j = 0; j < mu->bones.size(); ++j) {
                elements[i].bone_indices[j] = owner->name_to_node[mu->bones[j]->name];
            }
            elements[i].mesh = mu->mesh;
            elements[i].inverse_bind_transforms = mu->inverse_bind_transforms;
            elements[i].material = mu->material;
        }
    }

    void reflect() override {
        type_register<Element>("SkinPrototypeElement")
            .prop("material", &Element::material)
            .prop("mesh", &Element::mesh)
            .prop("bone_indices", &Element::bone_indices)
            .prop("inverse_bind_transforms", &Element::inverse_bind_transforms);
        type_register<mdlSkinComponentPrototype>("SkinPrototype")
            .parent<mdlComponentPrototype>()
            .prop("elements", &mdlSkinComponentPrototype::elements);
    }
};
struct mdlSkinComponentInstance : public mdlComponentInstance {
    TYPE_ENABLE(mdlComponentInstance);
    
    std::vector<std::unique_ptr<scnSkin>> objects;

    void make(mdlModelInstance* owner, mdlComponentPrototype* p) {
        LOG_WARN("mdlSkinComponentInstance");
        mdlSkinComponentPrototype* proto = (mdlSkinComponentPrototype*)p;
        objects.resize(proto->elements.size());
        for (int i = 0; i < objects.size(); ++i) {
            objects[i].reset(new scnSkin);
            objects[i]->setMeshDesc(proto->elements[i].mesh->getMeshDesc());
            objects[i]->setMaterial(proto->elements[i].material.get());
            objects[i]->setBoneIndices(proto->elements[i].bone_indices.data(), proto->elements[i].bone_indices.size());
            objects[i]->setInverseBindTransforms(proto->elements[i].inverse_bind_transforms.data(), proto->elements[i].inverse_bind_transforms.size());
            objects[i]->setSkeleton(&owner->scn_skeleton);
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
        type_register<mdlSkinComponentInstance>("SkinInstance")
            .parent<mdlComponentInstance>();
    }
};
typedef ComponentType<mdlSkinComponentMutable, mdlSkinComponentPrototype, mdlSkinComponentInstance> mdlSkinComponent;
