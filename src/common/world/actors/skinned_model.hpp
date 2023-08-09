#pragma once

#include "game/world/world.hpp"
#include "assimp_load_scene.hpp"

class wSkinnedModel : public Actor {
    gpuSkinModel* model = 0;
    SkeletonInstance skel_inst;

    std::set<scnMeshObject*> mesh_objects;

    void onSpawn(GameWorld* world) override {
        auto scn = world->getRenderScene();

        
    }
    void onDespawn(GameWorld* world) override {

    }
public:
    void setModel(gpuSkinModel* model) {
        this->model = model;

        for (int i = 0; i < model->simple_meshes.size(); ++i) {
            auto& msh = model->simple_meshes[i];
            // TODO
        }
    }
};
