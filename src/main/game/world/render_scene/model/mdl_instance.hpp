#pragma once

#include "mdl_prototype.hpp"

#include "mdl_component_instance.hpp"

struct mdlModelInstance {
    scnSkeleton scn_skeleton;
    std::vector<std::unique_ptr<mdlComponentInstance>> components;

    void make(mdlModelPrototype* proto) {
        scn_skeleton.parents = proto->parents;
        scn_skeleton.local_transforms = proto->default_local_transforms;
        scn_skeleton.world_transforms = scn_skeleton.local_transforms;
        scn_skeleton.world_transforms[0] = gfxm::mat4(1.0f);

        for (auto& kv : proto->components) {
            auto type = kv.first;
            components.push_back(
                std::unique_ptr<mdlComponentInstance>(mdlComponentGetTypeDesc(type)->pfn_constructInstance())
            );
            components.back()->make(this, kv.second.get());
        }
    }

    void onSpawn(wWorld* world) {
        world->getRenderScene()->addSkeleton(&scn_skeleton);
        for (int i = 0; i < components.size(); ++i) {
            components[i]->onSpawn(world);
        }
    }
    void onDespawn(wWorld* world) {
        for (int i = 0; i < components.size(); ++i) {
            components[i]->onDespawn(world);
        }
        world->getRenderScene()->removeSkeleton(&scn_skeleton);
    }
};
